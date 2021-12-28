cd -- "$(find -iname logs -type d)"
cd ..
rm -rf -- "$(pwd -P)"
cd ..