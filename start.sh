gcc namedPipeProducer.c -o namedPipeProducer
gcc namedPipeConsumer.c -o namedPipeConsumer
# ./namedPipeProducer & ./namedPipeConsumer

gcc unnamedPipe.c -o unnamedPipe
# ./unnamedPipe

gcc socketProducer.c -o socketProducer -lpthread
gcc socketConsumer.c -o socketConsumer -lpthread
# ./socketProducer 5001 & ./socketConsumer 127.0.0.1 5001

gcc sharedProducer.c -o sharedProducer -lrt -pthread
gcc sharedConsumer.c -o sharedConsumer -lrt -pthread
# ./sharedProducer  & ./sharedConsumer 

SIZE=1000
CHOICE=-1

RED='\033[1;31m'
GREEN='\033[1;32m'
BLUE='\033[1;34m'
NC='\033[0m'

clear
echo "SPEED EFFICIENCY METER"
echo 
echo 
read -p "Enter the amount of MB you want to transfer (min 1MB - max 100MB): " SIZE

while [ "$SIZE" -gt 100 ] || [ "$SIZE" -lt 1 ];do 
clear
echo "SPEED EFFICIENCY METER"
echo 
echo -e "${RED}Error: Incorrect size. Please try again${NC}"
read -p "Enter the amount of MB you want to transfer (min 1MB - max 100MB): " SIZE
done

clear
echo "SPEED EFFICIENCY METER"
echo 
echo 
echo -e "${GREEN}${SIZE}MB${NC} will be transfered. "
echo
echo
echo "Select which model you want to test:"
echo
echo -e "${BLUE}1${NC} - Named Pipe"
echo -e "${BLUE}2${NC} - Unnamed Pipe"
echo -e "${BLUE}3${NC} - Socket"
echo -e "${BLUE}4${NC} - Shared Memory"
echo
echo -e "Press ${BLUE}0${NC} to quit"
echo
read -p "Enter your choice: " CHOICE
echo

while : ;do
case $CHOICE in

    0)
    exit 0
    ;;

    1)
    ./namedPipeProducer ${SIZE} & ./namedPipeConsumer ${SIZE}
    CHOICE=-1
    ;;

    2)
    ./unnamedPipe ${SIZE}
    CHOICE=-1
    ;;

    3)
    ./socketProducer ${SIZE} 5001 & ./socketConsumer ${SIZE} 127.0.0.1 5001
    CHOICE=-1
    ;;

    4)
    ./sharedProducer ${SIZE}  & ./sharedConsumer ${SIZE}
    CHOICE=-1
    ;;

    -1)
    echo
    read -p "Enter your choice: " CHOICE
    echo
    ;;

    *)
    clear
    echo "SPEED EFFICIENCY METER"
    echo 
    echo 
    echo -e "${GREEN}${SIZE}MB${NC} will be transfered. "
    echo
    echo
    echo "Select which model you want to test:"
    echo
    echo -e "${BLUE}1${NC} - Named Pipe"
    echo -e "${BLUE}2${NC} - Unnamed Pipe"
    echo -e "${BLUE}3${NC} - Socket"
    echo -e "${BLUE}4${NC} - Shared Memory"
    echo
    echo -e "Press ${BLUE}0${NC} to quit"
    echo
    read -p "Enter your choice: " CHOICE
    ;;
esac
done

