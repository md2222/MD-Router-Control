#!/bin/bash


echo "Uninstall MD Router Control"
echo "These files will be deleted:"
echo "/usr/local/bin/mdrctrl"
echo "/usr/share/applications/mdrctrl.desktop"
echo "/usr/share/pixmaps/mdrctrl.png"
echo ""

echo -n "continue? (y/n): "
 
read ans
 
if [ "$ans" = "y" ] 
then


rm -- "/usr/local/bin/mdrctrl"
rm -- "/usr/share/applications/mdrctrl.desktop"
rm -- "/usr/share/pixmaps/mdrctrl.png"


echo "done"
#read -p "Press [Enter] key to exit..."

else
echo "canceled"
fi


exit 0
