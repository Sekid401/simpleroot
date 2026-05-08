# simpleroot
A couple of Android Files that can get you Root in Android

# How To Install

Download Every File, Get adb, Put the installer in /data/local/tmp and put the rest in /data, run the installer and go to /data then run ./su or ./rootstarter_su

# PLEASE READ

This has been tested on a VM with NO SELinux and i dont know if it will brick your SELinux device or not, otherwise its very dangerous to run, The risks are:

Bricking Your Device (low)

Breaking Certain Functions (a bit higher)

Breaking Some Apps (only if you didnt put symlinks)

# Tips

Once you Get Temporary Root put every file except the installer in /data/adb if you wanna and create a service.d, it wont work probably and get autodeleted, Though not recommended as its not persistent somewhy
