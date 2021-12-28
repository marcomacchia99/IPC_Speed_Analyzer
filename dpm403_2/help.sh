echo
more  "$(find . -type f -name "project_description.txt")"
echo 
echo
find . -type f -name "description.txt" | xargs -I{} sh -c "cat {}; echo '';echo''" | more



