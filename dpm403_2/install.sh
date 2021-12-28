# install <pathname>

unzip -q source.zip -d $1

cd $1/namedPipe
gcc namedPipeProducer.c -o namedPipeProducer
gcc namedPipeConsumer.c -o namedPipeConsumer

cd ..
cd unnamedPipe
gcc unnamedPipe.c -o unnamedPipe

cd ..
cd socket
gcc socketProducer.c -o socketProducer -lpthread
gcc socketConsumer.c -o socketConsumer -lpthread

cd ..
cd shared_memory
gcc sharedProducer.c -o sharedProducer -lrt -pthread
gcc sharedConsumer.c -o sharedConsumer -lrt -pthread

cd ..
mkdir logs

echo install completed







