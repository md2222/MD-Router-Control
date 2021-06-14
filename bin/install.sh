#!/bin/bash


echo "Install MD Router Control"
echo "These files will be created:"
echo "/usr/local/bin/mdrctrl"
echo "/usr/share/applications/mdrctrl.desktop"
echo "/usr/share/pixmaps/mdrctrl.png"
echo ""

echo -n "Continue? (y/n): "
 
read ans
 
if [ "$ans" = "y" ] 
then


cp -i -- mdrctrl "/usr/local/bin/"
cp -i -- mdrctrl.desktop "/usr/share/applications/"
cp -i -- mdrctrl.png "/usr/share/pixmaps/"
chmod 755 /usr/local/bin/mdrctrl


echo "done"
#read -p "Press [Enter] key to exit..."

else
echo "canceled"
fi


exit 0
