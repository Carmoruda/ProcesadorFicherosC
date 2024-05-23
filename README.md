# Auditor de Procesos

## Instalación y ejecución

Para obtener el código fuente del proyecto, ejecute el siguiente comando en su terminal:

```bash
1. git clone https://github.com/Carmoruda/ProcesadorFicherosC.git
2. cd ./ProcesadorFicherosC/src
3. ./make
4. ./fileprocessor
```

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
│   ├── create_structure.sh
│   ├── create_users.sh
│   ├── file_processor.c
│   ├── fileprocessor (ejecutable)
│   ├── make
│   └── show_information.c
├── tests/
│   ├── generateData
│   └── simulate
├── .gitignore
└── README.md
```
