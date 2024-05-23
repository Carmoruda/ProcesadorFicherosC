#!/bin/bash

# Grupos de usuarios
groupadd ufvauditor
groupadd ufvauditores

# Usuarios con sus respectivos grupos
useradd -m -G ufvauditor userSU001
useradd -m -G ufvauditor userSU002
useradd -m -G ufvauditor userSU003
useradd -m -G ufvauditor userSU004

useradd -m -G ufvauditores userfp
useradd -m -G ufvauditores usermonitor

# Permisos para los usuarios de sucursales
chown userSU001:ufvauditor ../files_data/SU001
chmod 760 ../files_data/SU001

chown userSU002:ufvauditor ../files_data/SU002
chmod 760 ../files_data/SU002

chown userSU003:ufvauditor ../files_data/SU003
chmod 760 ../files_data/SU003

chown userSU004:ufvauditor ../files_data/SU004
chmod 760 ../files_data/SU004

# Permisos para userfp y usermonitor en sus directorios
chown userfp:ufvauditores ../../FileProcessor
chmod 710 ../../FileProcessor

chown usermonitor:ufvauditores ./show_information.c
chmod 710 ./show_information.c

# Permisos para userfp y usermonitor en el archivo de configuración
chown userfp:ufvauditores ../conf/fp.conf
chown usermonitor:ufvauditores ../conf/fp.conf
chmod 640 ../conf/fp.conf

# Permisos específicos para userfp y usermonitor en todos los archivos
find /home -type f -exec chmod u+rw,g+r,o-rwx {} \;
find ../../FileProcessor -type d -exec chmod 710 {} \;
find ./show_information.c -type d -exec chmod 710 {} \;

echo "Usuarios y permisos configurados correctamente."