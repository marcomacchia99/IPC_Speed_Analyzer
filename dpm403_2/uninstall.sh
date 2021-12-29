FOLDER="$(find -iname logs -type d)"

if [ -z "${FOLDER}" ]
then
    echo "Source is not unzipped yet."
else
    cd "$FOLDER"
    cd ..
    rm -rf -- "$(pwd -P)"
    cd ..
fi
