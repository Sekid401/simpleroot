#!/system/bin/sh
# RootStarter Boot Script - lives in /data/adb/service.d/

SUBIN=/data/adb/su
POLICY=/data/adb/rootstarter.cil
LOG=/data/adb/rootstarter.log

log() { echo "[$(date)] $1" >> $LOG; }

log "RootStarter boot triggered"

if [ ! -f "$SUBIN" ]; then
    log "ERROR: Su binary missing from $SUBIN"
    exit 1
fi

if [ -f "$POLICY" ]; then
    # FIX #8: selinuxfs/load is write-only — shell stdin redirect doesn't work.
    # Must write to it explicitly with cat or dd.
    # FIX #9: Capture injection result HERE, not after unrelated chmod/chown calls.
    if cat "$POLICY" > /sys/fs/selinux/load; then
        log "SELinux policy injected"
    else
        log "SELinux injection failed — attempting fallback"

        # FIX #10: Poll for ADB TLS server readiness instead of blind sleep 2
        setprop persist.adb.tls_server.enable 1
        attempts=0
        while [ $attempts -lt 10 ]; do
            sleep 1
            state=$(getprop init.svc.adbd)
            if [ "$state" = "running" ]; then
                log "ADB ready after ${attempts}s"
                break
            fi
            attempts=$((attempts + 1))
        done

        if [ $attempts -lt 10 ]; then
            adb shell "$SUBIN" --inject-policy
            log "Wireless ADB fallback triggered"
        else
            log "ADB fallback timed out"
        fi

        setprop persist.adb.tls_server.enable 0
        log "Wireless ADB fallback complete"
    fi
else
    log "WARNING: Policy file missing"
fi

chown root:root "$SUBIN"
chmod 4755 "$SUBIN"
log "RootStarter ready"