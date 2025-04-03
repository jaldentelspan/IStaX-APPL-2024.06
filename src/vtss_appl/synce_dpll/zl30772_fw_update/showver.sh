if [ -n "$1" ] 
then
  sed -n 5p "$1"
else
  sed -n 5p zl30772.firmware1.hex
fi