#include <stdint.h>
#include <assert.h>

#include "esp_cpu.h"
#include "esp_err.h"

#include "soc/soc.h"
#include "soc/soc_caps.h"
#include "soc/rtc_cntl_reg.h"

typedef struct
{
    int priority;
    esp_cpu_intr_type_t type;
    uint32_t flags[SOC_CPU_CORES_NUM];
} intr_desc_t;

// Note: We currently only have dual core targets, so the table initializer is hard coded
static intr_desc_t const intr_desc_table[SOC_CPU_INTR_NUM] =
{
    {1, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //0
    {1, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //1
    {1, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //2
    {1, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //3
    {1, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      0                               }}, //4
    {1, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //5
#if CONFIG_FREERTOS_CORETIMER_0
    {1, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //6
#else
    {1, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL,    ESP_CPU_INTR_DESC_FLAG_SPECIAL  }}, //6
#endif
    {1, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL,    ESP_CPU_INTR_DESC_FLAG_SPECIAL  }}, //7
    {1, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //8
    {1, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //9
    {1, ESP_CPU_INTR_TYPE_EDGE,     {0,                                 0                               }}, //10
    {3, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL,    ESP_CPU_INTR_DESC_FLAG_SPECIAL  }}, //11
    {1, ESP_CPU_INTR_TYPE_LEVEL,    {0, 0}}, //12
    {1, ESP_CPU_INTR_TYPE_LEVEL,    {0, 0}}, //13
    {7, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //14, NMI
#if CONFIG_FREERTOS_CORETIMER_1
    {3, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //15
#else
    {3, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL,    ESP_CPU_INTR_DESC_FLAG_SPECIAL  }}, //15
#endif
    {5, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL,    ESP_CPU_INTR_DESC_FLAG_SPECIAL  }}, //16
    {1, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //17
    {1, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //18
    {2, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //19
    {2, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //20
    {2, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //21
    {3, ESP_CPU_INTR_TYPE_EDGE,     {ESP_CPU_INTR_DESC_FLAG_RESVD,      0                               }}, //22
    {3, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 0                               }}, //23
    {4, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      0                               }}, //24
    {4, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //25
    {5, ESP_CPU_INTR_TYPE_LEVEL,    {0,                                 ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //26
    {3, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //27
    {4, ESP_CPU_INTR_TYPE_EDGE,     {0,                                 0                               }}, //28
    {3, ESP_CPU_INTR_TYPE_NA,       {ESP_CPU_INTR_DESC_FLAG_SPECIAL,    ESP_CPU_INTR_DESC_FLAG_SPECIAL  }}, //29
    {4, ESP_CPU_INTR_TYPE_EDGE,     {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //30
    {5, ESP_CPU_INTR_TYPE_LEVEL,    {ESP_CPU_INTR_DESC_FLAG_RESVD,      ESP_CPU_INTR_DESC_FLAG_RESVD    }}, //31
};

void esp_cpu_intr_get_desc(int core_id, int intr_nb, esp_cpu_intr_desc_t *intr_desc_ret)
{
    assert(core_id >= 0 && core_id < SOC_CPU_CORES_NUM);
#if SOC_CPU_CORES_NUM == 1
    core_id = 0;    //  If this is a single core target, hard code CPU ID to 0
#endif

    intr_desc_ret->priority = intr_desc_table[intr_nb].priority;
    intr_desc_ret->type = intr_desc_table[intr_nb].type;
    intr_desc_ret->flags = intr_desc_table[intr_nb].flags[core_id];
}

