
#include <stdint.h>
#include "stm32f4xx.h"

uint32_t SystemCoreClock = 168000000;
volatile uint32_t systick_counter = 0;

void clock_init(void) {
    // 1. HSE'yi aktif et
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));  // HSE hazır olana kadar bekle

    // 2. PLL ayarları (örnek: HSE = 8 MHz → SYSCLK = 168 MHz)
    RCC->PLLCFGR = (8 << RCC_PLLCFGR_PLLM_Pos)   |  // PLLM = 8
                   (336 << RCC_PLLCFGR_PLLN_Pos) |  // PLLN = 336
                   (0 << RCC_PLLCFGR_PLLP_Pos)   |  // PLLP = 2
                   (RCC_PLLCFGR_PLLSRC_HSE)      |  // PLL kaynağı HSE
                   (7 << RCC_PLLCFGR_PLLQ_Pos);     // PLLQ = 7 (USB için)

    // 3. PLL'i aktif et
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));  // PLL hazır olana kadar bekle

    // 4. Flash ayarları (wait state)
    FLASH->ACR |= FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN;
    FLASH->ACR |= FLASH_ACR_LATENCY_5WS;  // 168 MHz için 5 WS

    // 5. AHB, APB1, APB2 prescaler
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;     // AHB = SYSCLK / 1
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;    // APB1 = HCLK / 4 (max 42 MHz)
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;    // APB2 = HCLK / 2 (max 84 MHz)

    // 6. PLL'i sistem saat kaynağı olarak seç
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);  // PLL aktif oldu mu?
}

/*

Amaç STM32F407 mikrodenetleyicisini harici osilatör (HSE) ile çalıştırıp, PLL üzerinden 168 MHz sistem saat frekansı elde etmektir.

RCC->CR: 		Clock Control Register
RCC_CR_HSEON:   HSE osilatörü (harici kristal) aktif edilir. Discovery kartta bu 8 MHz kristaldir.
RCC_CR_HSERDY:  HSE osilatörünün hazır olup olmadığını bildirir (hazır = 1)

Bu ayarlarla HSE = 8 MHz iken PLL çıkışı = 168 MHz olur
	PLLM = 8: HSE frekansı 8 MHz olduğundan, 1 MHz'e ölçekleme yapılır.
	PLLN = 336: 1 MHz × 336 = 336 MHz VCO çıkışı
	PLLP = 2: Sisteme 336 / 2 = 168 MHz verilir.
	PLLSRC = HSE: PLL kaynak olarak harici kristali kullanır.
	PLLQ = 7: USB, SDIO gibi çevresel saatlerin oluşturulmasında kullanılır. (48 MHz: 336 / 7 ≈ 48 MHz)


RCC_CR_PLLON: PLL'i etkinleştirir
RCC_CR_PLLRDY: PLL kilitlenip çıkış sinyali üretmeye başladığında 1 olur. Hazır beklenmelidir.

Yüksek saat hızlarında flash belleğe erişim süresi kritik hale gelir
	LATENCY_5WS: 168 MHz’de en az 5 wait state gereklidir (bkz: STM32F4 datasheet).
	ICEN: Instruction Cache enable
	DCEN: Data Cache enable
	PRFTEN: Prefetch buffer enable
	Bu ayarlar, flash erişimini hızlandırır ve stabilite sağlar.

Prescaler ayarları sistem saatini AHB, APB1 ve APB2 bus’larına böler
	HB: Ana veri yolu. Genellikle en yüksek frekansta çalışır. (168 MHz)
	APB1: Düşük hızlı çevresel birimler (TIM2-5, USART2, I2C1 vs.) → max 42 MHz
	APB2: Yüksek hızlı çevresel birimler (TIM1, USART1, SPI1 vs.) → max 84 MHz
	Bu bölme işlemleri, çevresel donanımların güvenli çalışmasını sağlar.

Sistem saat kaynağı seçim biti
	00 = HSI (default)
	01 = HSE
	10 = PLL
	SW: Sistem saat kaynağı seçim biti
	SWS: Seçilen sistem saat kaynağının hangi olduğunu bildirir (feedback)
	PLL seçilir ve sistem çekirdeği, tüm işlemciler 168 MHz ile çalışmaya başlar.

clock_init() fonksiyonu çalıştıktan sonra SystemCoreClock değişkeni güncellenmediği için
bazı CMSIS tabanlı delay fonksiyonları yanlış çalışabilir. Gerekirse elle güncelleyin: SystemCoreClock = 168000000;

*/

void gpio_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;  // GPIOD saatini aktif et

    GPIOD->MODER |= (1 << (12 * 2));  // PD12 -> Output
    GPIOD->OTYPER &= ~(1 << 12);     // Push-pull
    GPIOD->OSPEEDR |= (3 << (12 * 2)); // High speed
    GPIOD->PUPDR &= ~(3 << (12 * 2));  // No pull-up/pull-down
}

/*

RCC->AHB1ENR: AHB1 veri yolu üzerindeki çevresel birimlerin saat kontrol register'ıdır.
GPIODEN biti (bit 3) GPIOD portuna saat sinyalini sağlar.
Saat sinyali verilmeden GPIO register'larına erişim olmaz (kilitlidir).

MODER register'ı her pin için 2 bit yön belirler:
	00: Giriş
	01: Genel amaçlı çıkış
	10: Alternate function (ör. USART, SPI vs.)
	11: Analog

PD12 için:

MODER[25:24] → pin 12'ye denk gelir (12 × 2)
(1 << 24) → 01 → output olarak ayarlanır.
Bu ayar yapılmadan pin dışa sinyal veremez.

OTYPER[12] = 0: PD12 pini push-pull olarak yapılandırılır.
1 yapılırsa: Open-drain (yalnızca 0 yazabilir, 1 yazmak için harici pull-up gerekir)
Push-pull: Pin hem GND’ye hem Vcc’ye aktif sürülebilir. LED sürmek için uygundur.

OSPEEDR[25:24]: PD12'nin çıkış hızı ayarıdır.
	00: Düşük hız (~2 MHz)
	01: Orta hız
	10: Yüksek hız
	11: Çok yüksek hız
	3 << 24 → 11: Çok yüksek hız
	LED uygulamaları için düşük/orta hız yeterlidir ancak toggle işlemi hızlı yapılmak istenirse bu ayar yapılabilir.

PUPDR[25:24]: PD12 için dahili direnç ayarları:
	00: Hi-Z (dirençsiz, serbest)
	01: Pull-up
	10: Pull-down
	11: Rezerve (kullanılmaz)
	Burada: (3 << 24) maskeleme ile 00 yapılır → hiçbir direnç etkin değil
	Çıkış pini olduğu için bu ayar genelde gereksizdir ama register’ı sıfırlamak güvenli bir alışkanlıktır.

Bu ayarların ardından, artık GPIOD->ODR |= (1 << 12); komutu ile PD12'yi HIGH yapabilir veya ^= ile toggle edebilirsiniz.
Discovery kartta bu pin LED'e bağlıdır (yeşil LED).

*/

void SysTick_Handler(void) {
    systick_counter++;
}

void systick_init(void) {
    // SystemCoreClock varsayılan 168 MHz olarak kabul ediliyor
    SysTick->LOAD  = (SystemCoreClock / 1000) - 1;  // 1ms per interrupt
    SysTick->VAL   = 0;                             // Sayacı sıfırla
    SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |  // Dahili CPU clock kullan
                     SysTick_CTRL_TICKINT_Msk   |  // Interrupt aktif
                     SysTick_CTRL_ENABLE_Msk;      // SysTick aktif
}

void delay_ms(uint32_t ms) {
    uint32_t start = systick_counter;
    while ((systick_counter - start) < ms);
}

/*

SysTick (System Timer), ARM Cortex-M çekirdekleri tarafından donanımsal olarak sağlanan ve zamanlayıcı (timer) amaçlı kullanılan özel bir 32-bit sayaçtır.
STM32 gibi Cortex-M tabanlı mikrodenetleyicilerde kesme (interrupt) üretimi, zaman gecikmeleri (delay) ve zaman ölçümü gibi işler için kullanılır.
Donanımsal bir zamanlayıcıdır.
Cortex-M3/M4 gibi çekirdeklerde yerleşik olarak gelir (yani STM32'ye özel değil, ARM mimarisi özelliğidir).
Genelde 1 ms periyotta kesme üretmek için yapılandırılır.
Core Peripherals kapsamında yer alır ve core_cm4.h (CMSIS) dosyasında tanımlıdır.

SysTick->CTRL
	Kontrol ve durum kaydı (Control and Status Register)
		Bit	Adı	Açıklama
			0	ENABLE	Sayaç çalışsın mı? (1: Aktif, 0: Pasif)
			1	TICKINT	Sayaç sıfırlanınca kesme üret (1: evet)
			2	CLKSOURCE	Clock kaynağı (1: HCLK, 0: HCLK/8)
			16	COUNTFLAG	Sayaç sıfırlandıysa 1 olur

✅ SysTick->CALIB
	Kullanıcı kalibrasyonu (genellikle ignore edilir; fabrika değeri içerir)

✅ SysTick->VAL
	Anlık sayaç değeri
	Anlık olarak ne kadar kaldığını gösterir.

✅ SysTick->LOAD:
	Sayaç sıfıra ulaşınca bu değere yeniden yüklenir.
	Hedef gecikme süresine göre yüklenir. Burada amaç her 1 ms’de bir kesme üretmek.
	168 MHz için:
		168000000 / 1000 - 1 = 167999
 		Bu sayede 1 ms’de bir SysTick_Handler() çağrılır.

✅ SysTick_Handler():
	SysTick zamanlayıcı 0’a ulaştığında otomatik olarak çağrılır.
	Her çağrıldığında systick_counter artar.

✅ delay_ms():
	Gecikmenin başında systick_counter değeri alınır.
	Hedef süre kadar artış beklenir.

SysTick timer: Cortex-M çekirdeği içinde 24-bit'lik özel bir zamanlayıcıdır.
SysTick->LOAD: Kaç clock sonra interrupt üretileceği ayarlanır
SysTick->VAL: Sayaç değeri
SysTick->CTRL: SysTick yapılandırma kaydı

Çalışma mantığı adım adım:
	1 - SysTick->LOAD = (SystemCoreClock / 1000) - 1;
		→ 168 MHz sistemde bu değer 167999 olur
		→ Böylece her 1 ms'de bir kesme üretilir.

	2 - SysTick->CTRL şu alanları aktif eder:
		ENABLE: Sayaç aktif
		TICKINT: Interrupt etkin
		CLKSOURCE: Clock kaynağı olarak işlemci saatini kullan (AHB)
	3 - Kesme oluştuğunda, SysTick_Handler() çağrılır
	4 - delay_ms(x) fonksiyonu çağrıldığında:
		Mevcut systick_counter değeri alınır (start)
		Sayaç x kadar artana kadar while döngüsünde beklenir


	1 - LOAD register’ına sayaç süresi yazılır.
			Örneğin 1 ms için SystemCoreClock / 1000.
	2 - VAL register’ı sıfırlanır (başlangıç).
	3 - CTRL ile:
			Sayaç başlatılır (ENABLE = 1)
			Clock kaynağı belirlenir (CLKSOURCE = 1 → HCLK)
			Kesme aktif edilir (TICKINT = 1)
	4 - Sayaç her clock döngüsünde bir azalır.
	5 - Sayaç 0 olduğunda:
			COUNTFLAG 1 olur
			VAL, LOAD’dan tekrar yüklenir
			(Eğer TICKINT=1 ise) SysTick interrupt oluşur

Bu yöntem, her 1 ms’de bir gerçekleşen donanım kesmesi sayesinde gecikmeyi sağlar.
CPU döngüde bekler (bloklayıcı), ama sayaç kesme ile artar → daha doğru zamanlama.
DWT yöntemine göre daha az hassas ama daha düşük CPU yükü ile çalışır.

*/

int main(void) {
    clock_init();     // Sistemi 168 MHz'e ayarla
    gpio_init();      // GPIO başlat
    systick_init();    // SysTick timer başlat

    while (1) {
        GPIOD->ODR ^= (1 << 12);  // PD12 Toggle
        delay_ms(500);
    }
}
