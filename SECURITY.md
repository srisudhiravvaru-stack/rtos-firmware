# Security Policy

## Supported Versions

| Version | Supported |
|---|---|
| 1.x (main) | ✅ Active |
| 0.x (legacy) | ❌ End of life |

## Reporting a Vulnerability

**Do not open a public GitHub issue for security vulnerabilities.**

Please report security issues by emailing **security@your-org.com** with:

1. A description of the vulnerability and its potential impact
2. Steps to reproduce or proof-of-concept (if applicable)
3. Affected firmware versions or commit ranges
4. Any suggested mitigations

You will receive an acknowledgement within **48 hours** and a detailed response within **7 days**.

## Scope

This policy covers:
- Memory safety issues (buffer overflows, use-after-free)
- Stack overflow vulnerabilities
- Improper ISR/task boundary violations
- Watchdog bypass or suppression
- UART/MQTT input validation weaknesses

## Disclosure Policy

We follow **Coordinated Vulnerability Disclosure (CVD)**. We ask that you:
- Allow 90 days for a fix before public disclosure
- Avoid testing against production/deployed systems
- Not access, modify, or delete data beyond what is needed to demonstrate the issue
