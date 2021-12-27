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
MODE=0

RED='\033[1;31m'
GREEN='\033[1;32m'
BLUE='\033[1;34m'
NC='\033[0m'

get_dimension(){
    clear
    echo "SPEED EFFICIENCY METER"
    echo 
    echo 
    read -p "Enter the amount of MB you want to transfer (min 1MB - max 100MB): " SIZE

    while [ "$SIZE" -gt 100 ] || [ "$SIZE" -lt 1 ] || [ -z "$SIZE" ] || [[ ! "$SIZE" =~ ^[0-9]+$ ]] ;do 
    clear
    echo "SPEED EFFICIENCY METER"
    echo 
    echo -e "${RED}Error: Incorrect input. Please try again${NC}"
    read -p "Enter the amount of MB you want to transfer (min 1MB - max 100MB): " SIZE
    done
}

display_instructions(){



    clear
    echo "SPEED EFFICIENCY METER"
    echo 
    echo 
    echo -e "${GREEN}${SIZE}MB${NC} will be transferred. "
    echo
    echo
    echo "Select which model you want to test:"
    echo
    echo -e "${BLUE}1${NC} - Named Pipe"
    echo -e "${BLUE}2${NC} - Unnamed Pipe"
    echo -e "${BLUE}3${NC} - Socket"
    echo -e "${BLUE}4${NC} - Shared Memory"
    echo
    echo -e "Press ${BLUE}5${NC} to change size"
    echo
    if [[ "$MODE" -eq 0 ]]
    then
        echo -e "NOTE: every program will allocate the buffers using ${BLUE}dynamic memory${NC}."
        echo -e "Press ${BLUE}6${NC} to switch to ${BLUE}standard method${NC} (increasing stack size)."
    else
        echo -e "NOTE: every program will allocate the buffers using ${BLUE}standard method${NC}"
        echo "(increasing stack size)."
        echo -e "Press ${BLUE}6${NC} to switch to ${BLUE}dynamic memory${NC}."
    fi 
    echo
    echo -e "Press ${BLUE}0${NC} to quit"
    echo
    read -p "Enter your choice: " CHOICE
    echo
}

get_dimension
display_instructions


while : ;do
case $CHOICE in

    0)
    exit 0
    ;;

    1)
    ./namedPipeProducer ${SIZE} ${MODE} & ./namedPipeConsumer ${SIZE} ${MODE}
    CHOICE=-100
    ;;

    2)
    ./unnamedPipe ${SIZE} ${MODE}
    CHOICE=-100
    ;;

    3)
    # echo
    # read -p "Enter the: " CHOICE
    # echo
    ./socketProducer ${SIZE} ${MODE} 5555 & ./socketConsumer ${SIZE} ${MODE} 127.0.0.1 5555
    CHOICE=-100
    ;;

    4)
    ./sharedProducer ${SIZE} ${MODE} & ./sharedConsumer ${SIZE} ${MODE}
    CHOICE=-100
    ;;

    5)
    get_dimension
    CHOICE=-99
    ;;

    6)
    if [[ "$MODE" -eq 0 ]]
    then
        MODE=1
    else
        MODE=0
    fi
    
    CHOICE=-99
    ;;

    -100)
    echo
    read -p "Enter your choice: " CHOICE
    echo
    ;;

    *)
    display_instructions
    ;;
esac
done

