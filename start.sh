# gcc namedPipeProducer.c -o namedPipeProducer
# gcc namedPipeConsumer.c -o namedPipeConsumer
# ./namedPipeProducer & ./namedPipeConsumer

# gcc unnamedPipe.c -o unnamedPipe
# ./unnamedPipe

# gcc socketProducer.c -o socketProducer -lpthread
# gcc socketConsumer.c -o socketConsumer -lpthread
# ./socketProducer 5001 & ./socketConsumer 127.0.0.1 5001

gcc sharedProducer.c -o sharedProducer -lrt -pthread
gcc sharedConsumer.c -o sharedConsumer -lrt -pthread
./sharedProducer  & ./sharedConsumer 