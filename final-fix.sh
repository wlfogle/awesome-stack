#!/bin/bash
#
# Final fix script to handle shell differences and verify all containers.

# --- Configuration ---
PROXMOX_HOST="proxmox"
CONTAINERS_TO_CHECK=("100" "103" "104" "106" "107" "220" "224" "230" "231" "234" "260" "274" "900" "950")

# --- Colors ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# --- Functions ---

# Execute a command in a container, trying bash first, then sh.
container_exec() {
    local ct_id=$1
    shift
    local cmd_to_run="$*"
    
    # Try with bash first
    ssh "$PROXMOX_HOST" "pct exec $ct_id -- bash -c '$cmd_to_run'" 2>/dev/null || \
    ssh "$PROXMOX_HOST" "pct exec $ct_id -- sh -c '$cmd_to_run'" 2>/dev/null || \
    echo "EXEC_ERROR"
}

# Final verification of a single container
verify_container() {
    local ct_id=$1
    echo -e "${BLUE}--- Verifying Container $ct_id ---${NC}"

    local status
    status=$(container_exec "$ct_id" "systemctl is-system-running --wait")

    if [[ "$status" == "EXEC_ERROR" ]]; then
        echo -e "  ${RED}✗ Failed to execute commands in container. May be damaged or inaccessible.${NC}"
        return
    fi

    case "$status" in
        running|online)
            echo -e "  ${GREEN}✓ OK: Systemd is running correctly.${NC}"
            ;;
        degraded)
            echo -e "  ${YELLOW}⚠ WARN: Systemd is degraded. Resetting failed units again.${NC}"
            container_exec "$ct_id" "systemctl reset-failed"
            local final_status
            final_status=$(container_exec "$ct_id" "systemctl is-system-running --wait")
            echo -e "  Final status: $final_status"
            ;;
        stopping|starting)
            echo -e "  ${YELLOW}INFO: Systemd is stopping or starting. This may be temporary.${NC}"
            ;;
        *)
            echo -e "  ${RED}✗ FAIL: Systemd status is '$status'. Manual investigation needed.${NC}"
            ;;
    esac
}

# --- Main Execution ---

echo "Starting final verification script..."

for ct in "${CONTAINERS_TO_CHECK[@]}"; do
    verify_container "$ct"
    echo
done

echo "Verification finished."

