# install <pathname>

if [ -z "${1}" ]
then
    echo "Please, provide the folder path name as parameter."
else

    FOLDER="$(find -iname logs -type d)"
    
    if [ -z "${FOLDER}" ]
    then

        unzip -q source.zip -d $1

        cd $1/named_pipe
        gcc namedPipeProducer.c -o namedPipeProducer
        gcc namedPipeConsumer.c -o namedPipeConsumer

        cd ..
        cd unnamed_pipe
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
    else
        echo "The source archive has already been unzipped."
    fi

fi







