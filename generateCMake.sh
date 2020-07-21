#/bin/bash
FILE="CMakeLists.txt"
FIRSTTIME="YES"
> $FILE
find . -print | grep \.cpp$ | while read line
do
	if [ $FIRSTTIME == YES ] 
	then
		echo "set(srcs \"$line\"" >> $FILE
		FIRSTTIME="NO"
	else
		echo "	\"$line\"" >> $FILE
	fi
done
echo ")" >> $FILE

echo "" >> $FILE
echo "idf_component_register(SRCS \"\${srcs}\"" >> $FILE
echo "		REQUIRES mbedtls nvs_flash json mdns fatfs" >> $FILE
echo "		INCLUDE_DIRS ." >> $FILE
echo "		)" >> $FILE
