#!/bin/bash

echo
echo -e "\e[31mM\e[32ma\e[33mg\e[36mi\e[37mc\e[0m Cutter -- by mHi"
echo

if [ $# -ne 1 ] ; then
    echo "Usage: $0 [path/]I-Magic.sys"
    echo
    echo You may find it under 'c:\windows\system32\driver\I-Magic.sys' "(somtimes named ezusb.sys)"
    echo or if you have a RLV or any other Tacx DVD have a look at
    echo {PATH_TO_DVD}/FortiusInstall/support/driver_imagic/I-Magic.sys
    echo
    exit 1
fi

# Tacx IMagic
USB_VID_imagic="3561"
USB_PID_imagic="1902"

# Anchor EZUSB: 
# USB_VID_imagic_new="0547" 
# USB_PID_imagic_new="2131"

outfile="tacximagic.hex"
outfile_loader="ezloader.hex"
infile="$1"


# md5-driver, md5-imagic-driver-bin , md5-imagic-drier-hex, offset-in-driver, number-of-codelines(22b)
versions=( 
 "b340cab9e247ca86001e039be6580820 cbf25dec731e31ec84430ece9ef560cc 518ae7c8bc7ea54325ee84327d3135d7
  9248 268 15144 139 OLD_1A_Version" 
 "abb1c6e9173d3732b9df97298a653466 76e08829ef4588ccac400203dcaca677 b9409bfb4650e4356fc3c515aadb6aba
  9248 277 15344 139 NEW_1C_Version1"
 "da713b313e4fcd94097305ffde9bdde7 e8d6efca70481d8e5a9ef7ce12c37526 803be5bdde2e29390a6e7dd5e2ddc4cf 
  9248 274 15280 139 NEW_1C_Version2" 
  )
md5loader="0337c62cb1b93678d38792cce6fd0ad4"
	
md5=($(md5sum "$infile"))

for i in "${versions[@]}" ; do
 arr=( ${i} )
 if [ "$md5" == "${arr[0]}" ]; then
   echo -e "\e[32m" Found "\e[0m" Sys-Md5 ${arr[7]} !
   md5sys=${arr[0]}
   md5dd=${arr[1]}
   md5hex=${arr[2]}
   dd_start=${arr[3]}
   dd_count=$((22*${arr[4]}))
   dd_loader_start=${arr[5]}
   dd_loader_count=$((22*${arr[6]}))
 fi
done

if [ "$md5sys" == "" ] ; then
  echo -e "\e[31m" "$infile" : initial md5 sum mismatch "\e[0m"
  exit
fi

md5=($(dd if="$infile" bs=1 skip=$dd_start count=$dd_count status=none | md5sum))
if [ $md5 != $md5dd ]; then
  echo -e "\e[31m   " dd: cutted firmware md5 sum mismatch "\e[0m"
  echo $md5dd $md5
  exit
fi
echo -e ... DD-Md5 is "\e[32m" OK! "\e[0m"

function bin2hex() {
( dd if="$1" bs=1 skip=$3 count=$4 status=none | od -t u1 -w22 -An | while read -r -a b ; do 
  cnt=${b[0]}
  cnt=$((cnt+4))

  printf ":%02X%02X%02X" ${b[0]} ${b[3]} ${b[2]} 
  for i in $(seq 4 $cnt); do printf %02X ${b[$((i))]}  ; done

  c=0; for i in $(seq 0 $cnt); do c=$((c+${b[i]})); done; 
  printf "%02X\n" $(((~c+1)&255))
done) > "$2"
}

bin2hex "$infile" "$outfile" $dd_start $dd_count
bin2hex "$infile" "$outfile_loader" $dd_loader_start $dd_loader_count
echo
echo Written "$outfile"
echo Written "$outfile_loader" "(just in case)"
echo

md5=($(md5sum "$outfile"))
if [ $md5 != $md5hex ]; then
  echo -e "\e[31m" "$outfile" : Final md5 sum mismatch "\e[0m"
  echo $md5hex $md5
  exit
fi

echo -e ... HEX-Md5 is "\e[32m" OK! "\e[0m"

md5=($(md5sum "$outfile_loader"))
if [ $md5 != $md5loader ]; then
  echo -e "\e[31m" "$outfile_loader" : boot loader md5 sum mismatch "(not fatal)" "\e[0m"
  echo $md5hex $md5
fi


usbdev=($(lsusb | grep "$USB_VID_imagic:$USB_PID_imagic"))

bus=${usbdev[1]}
dev=${usbdev[3]}

if [ "${bus}" == "" ]; then
  echo
  echo -e "\e[31m" No IMagic Device under /dev/bus/usb/ ... do not forget to plug your device "\e[0m"
  echo
  exit
fi

echo
echo -e "\e[32m" Found "\e[0m" your IMagic Device under /dev/bus/usb/$bus/${dev%%:} 
echo

echo For testing and manually use, give the user write permissions and load the firmare with
echo -e "\e[36m   " sudo chmod 0666  /dev/bus/usb/$bus/${dev%%:} "\e[0m"
echo -e "\e[36m   " fxload -I "$outfile" -D /dev/bus/usb/$bus/${dev%%:} "\e[0m"
echo
echo "If everything is OK, the green LED is turned on (do not forget to power the trainer)"
echo 
echo "To permanently install the driver, copy firmware and the udev-rules with"
echo -e "\e[36m   "  sudo cp 90-tacximagic.rules  /etc/udev/rules.d/90-tacximagic.rules "\e[0m"
echo -e "\e[36m   "  sudo cp tacximagic.hex /lib/firmware/tacximagic.hex  "\e[0m"
echo
echo "Note: The original software uses a standard boot loader, but this is not necessary!"
echo "You only need the main $outfile file!"
echo "Just in case you have problems you may try:"
echo "fxload -s ezloader.hex -I imagic.hex -D /dev/bus/...."
echo

