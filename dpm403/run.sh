#buffer size
SIZE=1000
#circular buffer size
CIRCULAR_SIZE=1000
#user input choice
CHOICE=-1
#memory mode
MODE=0
#port no for socket
PORTNO=51489

#alternative echo colours
RED='\033[1;31m'
GREEN='\033[1;32m'
BLUE='\033[1;34m'
NC='\033[0m'

#get buffer dimension
get_dimension(){
    clear
    echo "SPEED EFFICIENCY METER"
    echo 
    echo 
    read -p "Enter the amount of MB you want to transfer (min 1MB - max 100MB): " SIZE

    #while SIZE < 1 or SIZE > 100 or SIZE is null or SIZE is not a number
    while [ "$SIZE" -gt 100 ] || [ "$SIZE" -lt 1 ] || [ -z "$SIZE" ] || [[ ! "$SIZE" =~ ^[0-9]+$ ]] ;do 
    clear
    echo "SPEED EFFICIENCY METER"
    echo 
    echo -e "${RED}Error: Incorrect input. Please try again${NC}"
    read -p "Enter the amount of MB you want to transfer (min 1MB - max 100MB): " SIZE
    done
}

#get circular buffer dimension
get_circular_dimension(){
    echo
    read -p "        Enter the circular buffer size in KB  (min 1KB - max 10KB): " CIRCULAR_SIZE
    echo

    #while CIRCULAR_SIZE < 1 or CIRCULAR_SIZE > 10 or CIRCULAR_SIZE is null or CIRCULAR_SIZE is not a number
    while [ -z "$CIRCULAR_SIZE" ] || [ "$CIRCULAR_SIZE" -gt 10 ] || [ "$CIRCULAR_SIZE" -lt 1 ] || [[ ! "$CIRCULAR_SIZE" =~ ^[0-9]+$ ]] ;do 
    echo
    read -p "        Enter the circular buffer size in KB  (min 1KB - max 10KB): " CIRCULAR_SIZE
    echo
    done
}

#print program instruction
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
        echo -e "Press ${BLUE}6${NC} to switch to ${BLUE}standard array method${NC} (increasing stack size)."
    else
        echo -e "NOTE: every program will allocate the buffers using ${BLUE}standard array method${NC}"
        echo "(increasing stack size)."
        echo -e "Press ${BLUE}6${NC} to switch to ${BLUE}dynamic memory${NC}."
    fi 
    echo
    echo -e "Press ${BLUE}0${NC} to quit"
    echo
    read -p "Enter your choice: " CHOICE
    echo
}

cd -- "$(find . -iname logs -type d)"
get_dimension
display_instructions


#infinite loop
while : ;do
case $CHOICE in

    0)
    #quit program
    exit 0
    ;;

    1)
    #preserve precedent version of log and delete the older one
    cd ..
    cd logs
    rm named_pipe_log-old.txt -f
    mv named_pipe_log.txt named_pipe_log-old.txt -f 2>/dev/null
    cd ..
    cd named_pipe
    ./namedPipeProducer ${SIZE} ${MODE} & ./namedPipeConsumer ${SIZE} ${MODE}
    #introduce very small delay to guarantee correct user interfacing
    sleep 0.001
    #fixed value that asks the user for another input
    CHOICE=-100
    ;;

    2)
    #preserve precedent version of log and delete the older one
    cd ..
    cd logs
    rm unnamed_pipe_log-old.txt -f
    mv unnamed_pipe_log.txt unnamed_pipe_log-old.txt -f 2>/dev/null
    cd ..
    cd unnamed_pipe
    ./unnamedPipe ${SIZE} ${MODE}
    #introduce very small delay to guarantee correct user interfacing
    sleep 0.001
    #fixed value that asks the user for another input
    CHOICE=-100
    ;;

    3)
    #preserve precedent version of log and delete the older one
    cd ..
    cd logs
    rm socket_log-old.txt -f
    mv socket_log.txt socket_log-old.txt -f 2>/dev/null
    cd ..
    cd socket
    ./socketProducer ${SIZE} ${MODE} ${PORTNO} & ./socketConsumer ${SIZE} ${MODE} 127.0.0.1 ${PORTNO}
    #introduce very small delay to guarantee correct user interfacing
    sleep 0.001
    #fixed value that asks the user for another input
    CHOICE=-100
    ;;

    4)
    #preserve precedent version of log and delete the older one
    get_circular_dimension
    cd ..
    cd logs
    rm shared_memory_log-old.txt -f
    mv shared_memory_log.txt shared_memory_log-old.txt -f 2>/dev/null
    cd ..
    cd shared_memory
    ./sharedProducer ${SIZE} ${MODE} ${CIRCULAR_SIZE} & ./sharedConsumer ${SIZE} ${MODE} ${CIRCULAR_SIZE}
    #introduce very small delay to guarantee correct user interfacing
    sleep 0.001
    #fixed value that asks the user for another input
    CHOICE=-100
    ;;

    5)
    get_dimension
    #random value used to print again the instructions
    CHOICE=-99
    ;;

    6)
    #switch memory mode
    if [[ "$MODE" -eq 0 ]]
    then
        MODE=1
    else
        MODE=0
    fi
    #random value used to print again the instructions
    CHOICE=-99
    ;;

    -100)
    #ask for another input
    echo
    read -p "Enter your choice: " CHOICE
    echo
    ;;

    *)
    display_instructions
    ;;
esac
done