# Auditor de Procesos

## Estructura de directorios

```bash
Nombre del proyecto/
├── conf/
│   └──fp.conf
├── file_data/
│   ├── processed/
│   ├── SUC001/
│   │   └── SU001_OPE001_DDMMAAAA_No.data
│   ├── SU002/
│   │   └── SU002_OPE001_DDMMAAAA_No.data
│   ├── SU003/
│   │   └── SU003_OPE001_DDMMAAAA_No.data
│   └── SU004/
│       └── SU004_OPE001_DDMMAAAA_No.data
├── include/
│   ├── check_patterns.h
│   ├── error_messages.h
│   ├── program_data.h
│   └── show_information.h
├── output/
│   ├── fich_log.log
│   └── fich_consolidado.csv
├── src/
│   ├── check_patterns.c
│   ├── create_structure.sh
│   ├── file_processor.c
│   ├── file_processor (ejecutable)
│   ├── make
│   └── show_information.c
├── tests/
│   ├── generateData
│   └── simulate
├── .gitignore
└── README.md
```
