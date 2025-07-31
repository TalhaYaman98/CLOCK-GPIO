CLOCK-GPIO: STM32F407 CMSIS Clock ve GPIO Projesi

STM32F407 Discovery kartı için sadece CMSIS kütüphanesi kullanarak sistem clock yapılandırması, GPIO kontrolü ve SysTick timer implementasyonu gerçekleştiren bare-metal programlama projesi.

🎯 Proje Amacı

Bu proje, STM32F407VGT6 mikrodenetleyicisinde:
- **HSE + PLL ile 168MHz sistem saati** elde etme
- **GPIO PD12 pin kontrolü** (Discovery kartı yeşil LED)
- **SysTick timer ile hassas gecikme** fonksiyonu
- **Register seviyesinde programlama** yaklaşımını gösterir

HAL kütüphanesi kullanılmadan doğrudan CMSIS ile donanım programlaması yapılmıştır.

✨ Temel Özellikler

  - 🚀 **168MHz Sistem Saati** - HSE (8MHz) + PLL konfigürasyonu
  - ⚡ **Optimize Flash Ayarları** - 5 Wait State + Cache aktifleştirme
  - 🔧 **Prescaler Yapılandırması** - AHB/APB1/APB2 bus saatleri
  - 💡 **GPIO Kontrolü** - PD12 pini (Discovery yeşil LED)
  - ⏱️ **SysTick Timer** - 1ms hassasiyetinde gecikme sistemi
  - 📏 **Minimal Kod** - Sadece gerekli register işlemleri

🛠️ Teknik Detaylar

  Hedef Platform
  - **MCU**: STM32F407VGT6 (ARM Cortex-M4F)
  - **Kart**: STM32F4 Discovery
  - **Crystal**: 8MHz HSE
  - **IDE**: STM32CubeIDE
  - **Kütüphane**: Sadece CMSIS
  
  Clock Yapılandırması
  ```
  HSE (8MHz) → PLL → SYSCLK (168MHz)
  ├── AHB  : 168MHz (÷1)
  ├── APB1 : 42MHz  (÷4) - Timer2-5, USART2, I2C1
  └── APB2 : 84MHz  (÷2) - Timer1, USART1, SPI1
  ```
  
  PLL Hesaplaması
  ```
  VCO = HSE × (PLLN/PLLM) = 8MHz × (336/8) = 336MHz
  SYSCLK = VCO ÷ PLLP = 336MHz ÷ 2 = 168MHz
  USB CLK = VCO ÷ PLLQ = 336MHz ÷ 7 = 48MHz
  ```
  
  📁 Proje Yapısı
  
  ```
  CLOCK-GPIO/
  ├── Core/
  │   ├── Src/
  │   │   └── main.c                # Ana program (clock + gpio + systick)
  │
  ├── Drivers/
  │   └── CMSIS/                   # CMSIS kütüphane
  │       └── stm32f4xx.h          # CMSIS device header
  ├── Debug/                       # Debug build
  ├── flash.ld                     # Linker script
  └── .project                     # STM32CubeIDE
  ```

🚀 Kurulum ve Çalıştırma

  Gereksinimler
    - STM32CubeIDE (v1.8.0+)
    - STM32F4 Discovery kartı
    - ST-LINK/V2 debugger
    - USB kablosu

Adımlar

  1. **Projeyi klonlayın:**
     ```bash
     git clone https://github.com/TalhaYaman98/CLOCK-GPIO.git
     cd CLOCK-GPIO
     ```
  
  2. **STM32CubeIDE'de açın:**
     ```
     File → Import → Existing Projects into Workspace
     → CLOCK-GPIO klasörünü seçin
     ```
  
  3. **Build ve Flash:**
     ```
     Project → Build All (Ctrl+B)
     Run → Debug As → STM32 Cortex-M C/C++ Application
     ```
  
  4. **Sonuç:**
     - Discovery kartındaki yeşil LED (PD12) 500ms aralıklarla yanıp söner
     - Sistem 168MHz'de çalışır

Teorik Altyapı
    RCC (Reset Clock Control)
    HSE: Harici kristal osilatör (8MHz Discovery)
    PLL: Frekans çoklayıcı (Phase-Locked Loop)
    Prescaler: Frekans bölücü (/1, /2, /4, /8, /16)
  
GPIO Register Yapısı
    MODER: Pin modu (input/output/AF/analog)
    OTYPER: Çıkış tipi (push-pull/open-drain)
    OSPEEDR: Çıkış hızı (slew rate control)
    PUPDR: Pull-up/down direnç
    ODR: Çıkış data register
    BSRR: Bit set/reset register (atomic)
  
SysTick Architecture
    24-bit counter: 16.7M max count
    Prescaler: /1 veya /8
    Interrupt: NVIC ile entegre
    Calibration: 10ms referans
  
Öğrenme Hedefleri
    Clock tree yapılandırması
    PLL hesaplamaları ve kilitleme
    Flash wait state gerekliliği
    GPIO register bit manipülasyonu
    SysTick timer kullanımı
    Interrupt handling (NVIC)
    Memory mapping kavramları
    Cortex-M4 çekirdeği özellikleri
    
