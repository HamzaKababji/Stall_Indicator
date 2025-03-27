#include <stdint.h>

// ----- Memory-Mapped Hardware Addresses -----
// ADC Base and Offsets
#define ADC_BASE          0xFF204000
#define ADC_CH0_OFFSET    0x00   // ADC channel 0
#define ADC_CH1_OFFSET    0x04   // ADC channel 1 (writing to here enables auto-update)

// GPIO (JP1) Base and Offsets for the 10 green LEDs on pins 0-9
#define JP1_BASE          0xFF200060
#define JP1_DATA_OFFSET   0x00   // Data register
#define JP1_DIR_OFFSET    0x04   // Direction register

// Slide Switches Base (SW0 is on bit 0)
#define SW_BASE           0xFF200040

// ----- Delay Function -----
// Simple busy-wait delay to slow down the loop for observable LED changes.
void delay(void) {
    for (volatile int i = 0; i < 100000; i++) {
        ; // Do nothing
    }
}

int main(void)
{
    // Pointers for ADC registers (volatile because these are hardware registers)
    volatile uint32_t *adc_ch0_ptr  = (volatile uint32_t *)(ADC_BASE + ADC_CH0_OFFSET);
    volatile uint32_t *adc_ch1_ptr  = (volatile uint32_t *)(ADC_BASE + ADC_CH1_OFFSET);
    
    // Pointers for GPIO registers for JP1 (LED display)
    volatile uint32_t *jp1_data_ptr = (volatile uint32_t *)(JP1_BASE + JP1_DATA_OFFSET);
    volatile uint32_t *jp1_dir_ptr  = (volatile uint32_t *)(JP1_BASE + JP1_DIR_OFFSET);
    
    // Pointer for Slide Switches
    volatile uint32_t *sw_ptr       = (volatile uint32_t *)SW_BASE;
    
    // ----- Configure GPIO -----
    // Set pins 0 to 9 of JP1 as outputs (for the 10 LEDs).
    // 0x3FF in hex is binary 11 1111 1111, which sets bits 0..9 to output.
    *jp1_dir_ptr = 0x3FF;
    
    // ----- Initialize ADC -----
    // Write any value to channel 0 to update all ADC channels once.
    *adc_ch0_ptr = 1;
    // Write any value to channel 1 to enable continuous (auto-update) ADC conversion.
    *adc_ch1_ptr = 1;
    
    while (1)
    {
        // ----- Read the Slide Switch -----
        // Mask off SW0 (bit 0). When SW0 is low, use ADC channel 0; when high, use ADC channel 1.
        uint32_t sw0 = (*sw_ptr) & 0x1;
        
        uint32_t adc_read;
        if (sw0 == 0)
        {
            adc_read = *adc_ch0_ptr;
        }
        else
        {
            adc_read = *adc_ch1_ptr;
        }
        
        // ----- Check for ADC Conversion Completion -----
        // Bit 15 is set when conversion is complete.
        if (adc_read & 0x10000) // change to 0x8000 for hardware
        {
            // Extract the 12-bit ADC value (bits [11:0])
            uint32_t adc_value = adc_read & 0xFFF;
            
            // ----- Convert ADC Reading to LED Bar Graph -----
            // Scale the 12-bit value (0–4095) into a value between 0 and 10.
            uint32_t led_count = (adc_value * 10) / 4095;
            if (led_count > 10)
                led_count = 10;
            
            // Create a bar graph: for example, if led_count is 5, then we want bits 0 to 4 on.
            // (1 << led_count) gives a binary 1 shifted led_count times.
            // Subtracting 1 produces a mask of led_count 1's. (e.g., (1<<5)-1 = 0x1F)
            uint32_t led_mask = (led_count > 0) ? ((1 << led_count) - 1) : 0;
            
            // Write the mask to the GPIO data register to drive the LEDs.
            *jp1_data_ptr = led_mask;
        }
        // Optionally, if the conversion is not complete (bit15 is 0), you could
        // implement error handling or simply try again on the next loop iteration.
        
        // Delay so that LED changes are observable (especially in cpulator).
        delay();
    }
    
    // This return is never reached.
    return 0;
}
