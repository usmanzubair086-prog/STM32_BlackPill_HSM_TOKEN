## Version 1.0 Milestone, Shortcomings, & Future Roadmap

This section documents the current functional architecture of the STM32 Black Pill Hardware Security Module (HSM) Token, its current engineering limitations, and planned enhancements for future development cycles.

---

### What I have Built (v1.0 Accomplishments)

The initial release establishes a functional, non-volatile, hardware-isolated credential manager. The core codebase achieves the following objectives:

* **Dual-Action State Machine:** Implemented an asynchronous edge-triggered button parsing routine via GPIO pin **PA0**. Supports short presses (<2s) for local USB injection and long presses (≥2s) to invoke credential regeneration. Integrated visual state tracking through the onboard **PC13** LED.
* **Dual-Mode Hardware Entropy Engine:** Formed a custom analog noise Harvester utilizing the Least Significant Bit (LSB) variance of floating ADC input pin **PA1**. Developed two separate distribution paths:
    * `Entropy_Generate_Password`: Restricts mapping to a safe, printable ASCII character array for standard credential typing.
    * `Entropy_Generate_Bytes`: Yields raw, unconstrained binary data block structures spanning **0x00** to **0xFF** to feed cryptographic initializations.
* **Cryptographic Vault Wrapper:** Created an encapsulation layer around the processing core, isolating raw keys using symmetric cryptography. The system accepts dynamic parameters, ensuring cryptographic assets never reside in plain text within flash boundaries.
* **Non-Volatile Internal Storage:** Leveraged the STM32's embedded flash architecture, targeting **Flash Sector 5** for persistent non-volatile retention. Credentials survive complete hardware power-downs without external component dependencies.
* **Native USB HID Injection:** Optimized standard USB Device middleware configurations to enumerate as a trusted HID Keyboard class. Bypasses host operating system driver requirements for immediate data injection.
* **Clean Repository Pipeline:** Established a pristine project workspace by mapping out a strict `.gitignore` index, clearing out local IDE compilation debris (`Debug/`), and handling cross-platform line-ending translations (`core.autocrlf`).

---

### Current Shortcomings & Technical Limitations

While `v1.0` achieves core operational requirements, it features several constraints that limit its application in hardened, high-security environments:

* **Workaround Entropy Source:** Relying on the ambient noise of a floating analog ADC pin functions well for a development prototype, but it does not represent a certified True Random Number Generator (TRNG). Environmental electromagnetic shielding or consistent power rail noise can introduce structural bias into the entropy pool.
* **Flash Wear & Longevity:** Writing directly to internal STM32 Flash sectors lacks native wear-leveling algorithms. Flash Sector 5 has finite write/erase cycles (typically rated for 10,000 to 100,000 cycles). Continuous generation sequences will eventually degrade the physical sector.
* **Single-Slot Structural Constraint:** The current memory map and API are hardcoded to handle a single primary secret payload. The token cannot host or manage an extensive keyring matrix.
* **Absence of Active Physical Protection:** The hardware layout lacks active physical tamper-detection mechanisms (such as cryptographic zeroization lines, casing breach loops, or resistance to side-channel power analysis attacks).

---

### Future Enhancements & Roadmap

To evolve this token into a resilient, deployment-grade security tool, development will focus on the following implementations:

* **Advanced Entropy Whitening:** Pass the raw ADC-harvested byte arrays through a cryptographic hash function (such as SHA-256) or a Von Neumann correction algorithm to eliminate statistical bias and maximize bit-level unpredictability.
* **Multi-Slot Keyring Management:** Revamp the flash memory layout to host an indexed lookup matrix. This will allow the device to manage multiple distinct accounts, using advanced button interactions (e.g., double-clicks, triple-clicks) to navigate slots.
* **External Secure Hardware Integration:** Migrate credential storage from internal MCU flash to an external I2C/SPI secure element or dedicated EEPROM chip featuring built-in cryptographic hardware countermeasures and native wear-leveling control.
* **Desktop Companion Interface:** Design a cross-platform companion application (built using Python) that interfaces with the token using secure USB HID feature reports. This will enable users to manage slot labels, back up configurations securely, and review token diagnostics without ever exposing the master cryptographic keys to the host machine.
