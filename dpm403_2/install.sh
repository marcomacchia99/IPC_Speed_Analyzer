# install <pathname>

unzip -q source.zip -d $1

cd $1/motor_x
gcc motorX.c -o motorX

cd ..
cd motor_z
gcc motorZ.c -o motorZ

cd ..
cd command_console
gcc commandConsole.c -o commandConsole

cd ..
cd inspection_console
gcc inspectionConsole.c -o inspectionConsole

cd ..
cd starter
gcc starter.c -o starter

cd ..
cd watchdog
gcc watchdog.c -o watchdog

cd ..
mkdir logs

echo install completed