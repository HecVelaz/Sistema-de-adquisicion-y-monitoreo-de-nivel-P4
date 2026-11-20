#include "sdcard.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "driver/spi_common.h"
#include "driver/sdspi_host.h"

#include "sdmmc_cmd.h"

static const char *TAG = "SDCARD";

static sdmmc_card_t *card = NULL;
static bool sd_mounted = false;

// Main y task_pendientes tocan temp.csv: todo acceso a la SD pasa por aca
static SemaphoreHandle_t sd_mutex = NULL;

#define SD_MUTEX_TIMEOUT_MS 5000

#define CABECERA_DATOS "epoch,id,distancia_m,velocidad_ms,senal_db,estado\n"
#define CABECERA_TEMP  "Timestamp,nivel\n"

// ======================================================
// MUTEX
// ======================================================

static bool sd_tomar(void)
{
    if (!sd_mounted || sd_mutex == NULL)
    {
        return false;
    }

    if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(SD_MUTEX_TIMEOUT_MS)) != pdTRUE)
    {
        ESP_LOGE(TAG, "SD ocupada: no se pudo tomar el mutex");
        return false;
    }

    return true;
}

static void sd_soltar(void)
{
    xSemaphoreGive(sd_mutex);
}

// ======================================================
// AUXILIARES (sin mutex: las llama quien ya lo tiene)
// ======================================================

static bool existe(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

// Cierra asegurando que los datos quedaron escritos en la tarjeta
// (sin fsync, un corte de energia puede perder lo que estaba en el buffer)
static bool cerrar_seguro(FILE *f)
{
    bool ok = (fflush(f) == 0);
    ok = (fsync(fileno(f)) == 0) && ok;
    ok = (fclose(f) == 0) && ok;
    return ok;
}

static bool asegurar_cabecera(const char *path, const char *cabecera)
{
    if (existe(path))
    {
        return true;
    }

    FILE *f = fopen(path, "w");

    if (f == NULL)
    {
        ESP_LOGE(TAG, "No se pudo crear %s", path);
        return false;
    }

    fputs(cabecera, f);

    if (!cerrar_seguro(f))
    {
        ESP_LOGE(TAG, "Error escribiendo cabecera de %s", path);
        return false;
    }

    ESP_LOGI(TAG, "%s creado con cabecera", path);
    return true;
}

static bool agregar_linea(const char *path, const char *linea)
{
    FILE *f = fopen(path, "a");

    if (f == NULL)
    {
        ESP_LOGE(TAG, "No se pudo abrir %s", path);
        return false;
    }

    fputs(linea, f);

    if (!cerrar_seguro(f))
    {
        ESP_LOGE(TAG, "Error escribiendo en %s", path);
        return false;
    }

    return true;
}

// ======================================================
// RECUPERAR LA COLA TRAS UN CORTE DE ENERGIA
// sdcard_remove_first_pending escribe temp_aux.csv completo (con fsync),
// recien despues borra temp.csv y renombra aux -> temp. Si la energia se
// corta en el medio:
//   - aux y temp existen: temp esta intacto -> se descarta aux
//   - solo aux existe:    aux esta completo -> se renombra a temp
// ======================================================

static void recuperar_cola(void)
{
    if (!existe(TEMP_AUX_FILE))
    {
        return;
    }

    if (existe(TEMP_QUEUE_FILE))
    {
        remove(TEMP_AUX_FILE);
        ESP_LOGW(TAG, "Recuperacion: se descarto temp_aux.csv (temp.csv intacto)");
    }
    else if (rename(TEMP_AUX_FILE, TEMP_QUEUE_FILE) == 0)
    {
        ESP_LOGW(TAG, "Recuperacion: temp_aux.csv renombrado a temp.csv");
    }
    else
    {
        ESP_LOGE(TAG, "Recuperacion: no se pudo renombrar temp_aux.csv");
    }
}

// ======================================================
// DETECTAR TARJETA
// ======================================================

bool detectar_tarjeta(void)
{
    gpio_reset_pin(SD_PIN_DET);
    gpio_set_direction(SD_PIN_DET, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SD_PIN_DET, GPIO_PULLUP_ONLY);

    return gpio_get_level(SD_PIN_DET) == 0;
}

// ======================================================
// INICIALIZAR SD
// ======================================================

bool sdcard_init(void)
{
    if (sd_mounted)
    {
        return true;
    }

    if (sd_mutex == NULL)
    {
        sd_mutex = xSemaphoreCreateMutex();
    }

    if (!detectar_tarjeta())
    {
        ESP_LOGW(TAG, "No hay tarjeta SD detectada");
        return false;
    }

    esp_err_t ret;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = 10000;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_PIN_MOSI,
        .miso_io_num = SD_PIN_MISO,
        .sclk_io_num = SD_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000
    };

    ret = spi_bus_initialize(
        host.slot,
        &bus_cfg,
        SDSPI_DEFAULT_DMA
    );

    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "Error inicializando SPI: %s", esp_err_to_name(ret));
        return false;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_PIN_CS;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    ret = esp_vfs_fat_sdspi_mount(
        MOUNT_POINT,
        &host,
        &slot_config,
        &mount_config,
        &card
    );

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error montando SD: %s", esp_err_to_name(ret));
        return false;
    }

    sd_mounted = true;

    ESP_LOGI(TAG, "SD montada correctamente");
    sdmmc_card_print_info(stdout, card);

    if (!sd_tomar())
    {
        return false;
    }

    recuperar_cola();

    bool ok = asegurar_cabecera(DATOS_FILE, CABECERA_DATOS) &&
              asegurar_cabecera(TEMP_QUEUE_FILE, CABECERA_TEMP);

    sd_soltar();

    return ok;
}

// ======================================================
// GUARDAR MEDICION COMPLETA EN datos.csv
// ======================================================

bool sdcard_guardar_medicion(uint32_t epoch, uint32_t id, float distancia_m,
                             float velocidad_ms, int senal_db, int estado)
{
    char dist[16] = "";
    char vel[16]  = "";
    char senal[12] = "";

    if (!isnan(distancia_m))  snprintf(dist, sizeof(dist), "%.3f", distancia_m);
    if (!isnan(velocidad_ms)) snprintf(vel, sizeof(vel), "%.3f", velocidad_ms);
    if (senal_db >= 0)        snprintf(senal, sizeof(senal), "%d", senal_db);

    char linea[96];
    snprintf(linea, sizeof(linea), "%lu,%lu,%s,%s,%s,%d\n",
             (unsigned long)epoch, (unsigned long)id, dist, vel, senal, estado);

    if (!sd_tomar())
    {
        ESP_LOGW(TAG, "SD no disponible: no se guardo la medicion");
        return false;
    }

    bool ok = asegurar_cabecera(DATOS_FILE, CABECERA_DATOS) &&
              agregar_linea(DATOS_FILE, linea);

    sd_soltar();

    if (ok)
    {
        ESP_LOGI(TAG, "Guardado SD: %.*s", (int)strlen(linea) - 1, linea);
    }

    return ok;
}

// ======================================================
// GUARDAR PENDIENTE EN temp.csv
// ======================================================

bool sdcard_append_pending(uint32_t timestamp, float level_m)
{
    char linea[32];
    snprintf(linea, sizeof(linea), "%lu,%.3f\n", (unsigned long)timestamp, level_m);

    if (!sd_tomar())
    {
        ESP_LOGW(TAG, "SD no disponible: pendiente perdido %lu,%.3f",
                 (unsigned long)timestamp, level_m);
        return false;
    }

    recuperar_cola();

    bool ok = asegurar_cabecera(TEMP_QUEUE_FILE, CABECERA_TEMP) &&
              agregar_linea(TEMP_QUEUE_FILE, linea);

    sd_soltar();

    if (ok)
    {
        ESP_LOGW(TAG, "Guardado pendiente: %lu,%.3f", (unsigned long)timestamp, level_m);
    }

    return ok;
}

// ======================================================
// LEER PRIMER PENDIENTE FIFO (false si la cola esta vacia)
// ======================================================

bool sdcard_read_first_pending(uint32_t *timestamp, float *level_m)
{
    if (timestamp == NULL || level_m == NULL || !sd_tomar())
    {
        return false;
    }

    recuperar_cola();

    bool ok = false;
    FILE *f = fopen(TEMP_QUEUE_FILE, "r");

    if (f != NULL)
    {
        char line[128];
        unsigned long ts_temp;
        float level_temp;

        // Saltar cabecera
        fgets(line, sizeof(line), f);

        while (fgets(line, sizeof(line), f))
        {
            if (sscanf(line, "%lu,%f", &ts_temp, &level_temp) == 2)
            {
                *timestamp = (uint32_t)ts_temp;
                *level_m = level_temp;
                ok = true;
                break;
            }
        }

        fclose(f);
    }

    sd_soltar();
    return ok;
}

// ======================================================
// ELIMINAR PRIMER PENDIENTE FIFO
// Escritura segura: aux completo y sincronizado -> borrar temp -> renombrar.
// Si se corta la energia en el medio, recuperar_cola() lo arregla al arrancar.
// ======================================================

bool sdcard_remove_first_pending(void)
{
    if (!sd_tomar())
    {
        return false;
    }

    recuperar_cola();

    bool ok = false;
    FILE *src = fopen(TEMP_QUEUE_FILE, "r");
    FILE *dst = NULL;

    if (src == NULL)
    {
        goto fin;
    }

    dst = fopen(TEMP_AUX_FILE, "w");

    if (dst == NULL)
    {
        fclose(src);
        goto fin;
    }

    char line[128];

    // Copiar cabecera
    if (fgets(line, sizeof(line), src))
    {
        fputs(line, dst);
    }
    else
    {
        fputs(CABECERA_TEMP, dst);
    }

    bool skipped = false;

    while (fgets(line, sizeof(line), src))
    {
        if (!skipped && strlen(line) > 3)
        {
            skipped = true;
            continue;
        }

        fputs(line, dst);
    }

    fclose(src);

    if (!cerrar_seguro(dst))
    {
        ESP_LOGE(TAG, "Error escribiendo temp_aux.csv; temp.csv sin cambios");
        remove(TEMP_AUX_FILE);
        goto fin;
    }

    remove(TEMP_QUEUE_FILE);

    if (rename(TEMP_AUX_FILE, TEMP_QUEUE_FILE) != 0)
    {
        ESP_LOGE(TAG, "No se pudo renombrar temp_aux.csv");
        goto fin;
    }

    ok = true;
    ESP_LOGI(TAG, "Primer pendiente eliminado");

fin:
    sd_soltar();
    return ok;
}
