#!/system/bin/sh
# RootStarter Installer - Run once via Shizuku

SUBIN=/data/adb/su
SERVICED=/data/adb/service.d
POLICY=/data/adb/rootstarter.cil

echo "[*] RootStarter Installer"

mkdir -p /data/adb/service.d

if [ -f /dev/rootstarter_su ]; then
    cp /dev/rootstarter_su "$SUBIN"
    chown root:root "$SUBIN"
    chmod 4755 "$SUBIN"
    echo "[+] Su binary installed to $SUBIN"
else
    echo "[-] Su binary not found in RAM, aborting"
    exit 1
fi

# FIX #11: Heredoc policy was missing type declaration entirely — su_exec
# was used in allow rules but never declared, causing load failure.
# FIX #12: typetransition target was 'su' (typo) — corrected to 'su_exec'.
cat > "$POLICY" << 'EOF'
(type su_exec)
(typeattribute su_domain)
(typeattributeset su_domain (su_exec))
(allow shell su_exec (file (execute read open getattr)))
(allow shell su_exec (process (transition)))
(allow su_exec shell (fd (use)))
(allow su_exec self (capability (setuid setgid)))
(allow su_exec self (process (setcurrent)))
(typetransition shell su_exec process su_exec)
EOF

# FIX #13: Same selinuxfs write bug as rootstarter.sh — use cat, not redirect
if cat "$POLICY" > /sys/fs/selinux/load; then
    echo "[+] SELinux policy injected"
else
    echo "[-] SELinux injection failed"
fi

cp /data/adb/rootstarter.sh "$SERVICED/rootstarter.sh"
chmod 755 "$SERVICED/rootstarter.sh"
echo "[+] Boot script installed"

echo "[*] Done. Reboot to verify persistence."