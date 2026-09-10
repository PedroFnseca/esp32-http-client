# Security Policy

## Supported Versions

Security updates are currently provided for the latest released version of ESP32-HTTP-Client.

| Version | Supported |
| ------- | --------- |
| Latest release | :white_check_mark: |
| Older releases | :x: |

If you are using an older version, please upgrade to the latest release before reporting a potential security issue whenever possible.

## Reporting a Vulnerability

If you discover a potential security vulnerability in ESP32-HTTP-Client, please do **not** disclose it publicly through a GitHub issue, discussion, or pull request.

Instead, report it privately through GitHub's private vulnerability reporting feature:

**Security Advisories → Report a vulnerability**

When reporting a vulnerability, please include as much of the following information as possible:

- A clear description of the vulnerability.
- The affected version(s).
- The ESP32 board or chip being used.
- The affected functionality or API.
- Steps to reproduce the issue.
- A minimal proof of concept, if available.
- Expected behavior.
- Actual behavior.
- Potential security impact.
- Any suggested mitigation or fix.

Please avoid including secrets, credentials, private URLs, API keys, or other sensitive information in the report.

## What to Report

Please report the following issues privately if you believe they may have a security impact:

* Memory corruption or out-of-bounds access that can be triggered by untrusted input.
* Buffer overflows or underflows that may be remotely or externally triggered.
* Use-after-free or other memory-safety issues with a potential security impact.
* Integer overflows that can result in memory corruption or other security consequences.
* Denial-of-service vulnerabilities that can be triggered remotely or through untrusted input.
* Improper handling of malformed HTTP responses that can lead to a security issue.
* Unsafe parsing of HTTP headers, URLs, or response bodies that can be exploited by an attacker.
* Authentication or authorization bypasses caused by the library.
* TLS or certificate validation flaws that could enable interception or impersonation.
* Unintentional exposure of sensitive information.
* Security issues involving redirects, URLs, external input, or untrusted network data.
* Any other issue that could allow an attacker to compromise, crash, or interfere with an ESP32 application through the use of ESP32-HTTP-Client.

### Regular Bugs

Not every bug is a security vulnerability.

For issues that do not have a security impact, such as:

* General bugs or unexpected behavior.
* Performance problems.
* Documentation issues.
* API improvements.
* Compatibility issues.
* Non-security-related crashes that cannot be triggered in a security-sensitive context.

please use the regular GitHub issue tracker or submit a pull request.

If you are unsure whether an issue has security implications, please report it privately. We can determine the appropriate disclosure path during the review.


## What Usually Does Not Constitute a Vulnerability

ESP32-HTTP-Client is a low-level HTTP client library and does not provide application-level security controls for every possible use case.

The following are generally the responsibility of the application using the library:

- Secure storage of API keys and credentials.
- Server-side authentication and authorization.
- Application-specific input validation.
- Protection of sensitive data returned by an API.
- Secure configuration of Wi-Fi credentials.
- Proper TLS certificate configuration when required by the application.
- Avoiding hardcoded secrets in firmware or source code.

However, if the library itself introduces a security weakness in these areas, please report it privately.

## Disclosure Process

After receiving a vulnerability report, the maintainers will:

1. Review and validate the report.
2. Determine the affected versions and security impact.
3. Develop and test a fix when applicable.
4. Release an updated version containing the fix.
5. Publish a security advisory when appropriate.

We ask security researchers to allow reasonable time for investigation and remediation before publicly disclosing the vulnerability.

## Security Updates

Security fixes will be included in new releases whenever possible.

Users are encouraged to:

- Keep ESP32-HTTP-Client updated.
- Review release notes and security advisories.
- Use TLS/HTTPS when communicating with services that require confidentiality or authentication.
- Avoid exposing sensitive credentials in source code or firmware.
- Validate and handle external data appropriately in the application layer.

## Scope

This policy applies to the ESP32-HTTP-Client source code and functionality maintained in this repository.

Third-party dependencies, ESP32 SDK/framework vulnerabilities, operating-system vulnerabilities, remote API vulnerabilities, and vulnerabilities in external services should generally be reported to their respective maintainers.

## Responsible Disclosure

We appreciate responsible security research and contributions that help make ESP32-HTTP-Client safer for the embedded and IoT community.

Thank you for helping improve the security of the project.
