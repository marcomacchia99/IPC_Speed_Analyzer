# gcc namedPipeProducer.c -o namedPipeProducer
# gcc namedPipeConsumer.c -o namedPipeConsumer
# ./namedPipeProducer & ./namedPipeConsumer

# gcc unnamedPipe.c -o unnamedPipe
# ./unnamedPipe

gcc socketProducer.c -o socketProducer
gcc socketConsumer.c -o socketConsumer
./socketProducer 5001 & ./socketConsumer 127.0.0.1 5001