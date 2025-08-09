# Awesome Stack

> **This repository now focuses on the current, supported media stack. Legacy scripts and docs are archived in `/legacy` and `/docs/_archive`.**

## 🚀 Quick Start

- **Stack Overview:** See [`docs/_organized/summary.md`](docs/_organized/summary.md)
- **Architecture:** Proxmox → VM → LXC → Docker → Traefik + Services
- **Automation:** Use the Ansible playbooks and roles in [`ansible/`](ansible/) for provisioning and rebuilds.
- **Main Entry Point:** [`rebuild-ultimate-stack.sh`](rebuild-ultimate-stack.sh) (see docs for full instructions)

## 🛠️ Documentation

- **Implementation/Plan:** [`docs/_organized/summary.md`](docs/_organized/summary.md)
- **AWX/Ansible workflow:** [`ansible/README.md`](ansible/README.md), [`docs/AWX_workflow_example.md`](docs/AWX_workflow_example.md)
- **Proxmox/LXC/KVM stack details:** [`Ansible/Recommended Stack for LXC + KVM Environments.md`](Ansible/Recommended%20Stack%20for%20LXC%20%2B%20KVM%20Environments.md)

## 🗃️ Legacy

- Moved to [`/legacy`](legacy/) and [`/docs/_archive`](docs/_archive/).

---

**If you need something from the old stack, look in `/legacy` or `/docs/_archive`. All current work should use the new structure and playbooks!**