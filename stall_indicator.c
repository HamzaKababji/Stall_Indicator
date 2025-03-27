#include <stdint.h>
#include <math.h>        // For sqrt() if you want to compute stall velocity at runtime

// ----- Memory-Mapped Hardware Addresses -----
// ADC Base and Offsets
#define ADC_BASE          0xFF204000
#define ADC_CH0_OFFSET    0x00   // ADC channel 0
#define ADC_CH1_OFFSET    0x04   // ADC channel 1 (writing to here enables auto-update)

// GPIO (JP1) Base and Offsets for the 10 green LEDs on pins 0-9
#define JP1_BASE          0xFF200060
#define JP1_DATA_OFFSET   0x00   // Data register
#define JP1_DIR_OFFSET    0x04   // Direction register

// Slide Switches Base (SW0 is on bit 0) — not strictly needed here
#define SW_BASE           0xFF200040

// ----- Aircraft / Aerodynamics Parameters -----
// Weight in Newtons for 100,000 kg (100,000 * 9.8 = 980,000 N)
#define WEIGHT        980000.0f 
#define RHO           1.225f      // Air density at sea level (kg/m^3)
#define WING_AREA     125.0f      // Wing area (m^2)
#define CL_MAX        1.2f        // Maximum lift coefficient
#define MAX_AOA       15.0f       // AoA limit for stall condition (degrees)

// ----- Delay Function -----
static void delay(void) {
    for (volatile int i = 0; i < 100000; i++) {
        ; // Do nothing
    }
}

int main(void)
{
    // Pointers for ADC registers (volatile because these are hardware registers)
    volatile int *adc_ch0_ptr  = (volatile int *)(ADC_BASE + ADC_CH0_OFFSET);
    volatile int *adc_ch1_ptr  = (volatile int *)(ADC_BASE + ADC_CH1_OFFSET);
    
    // Pointers for GPIO registers for JP1 (LED display)
    volatile int *jp1_data_ptr = (volatile int *)(JP1_BASE + JP1_DATA_OFFSET);
    volatile int *jp1_dir_ptr  = (volatile int *)(JP1_BASE + JP1_DIR_OFFSET);
    
    // (Optional) Pointer for Slide Switches, if needed
    // volatile uint32_t *sw_ptr       = (volatile uint32_t *)SW_BASE;
    
    // ----- Configure GPIO -----
    *jp1_dir_ptr = 0x3FF;  // Set pins 0..9 to output (for 10 LEDs)
    
    // ----- Initialize ADC -----
    *adc_ch0_ptr = 1; // Update all ADC channels once
    *adc_ch1_ptr = 1; // Enable continuous (auto-update) ADC conversion
    
    // ----- Compute Stall Velocity -----
    // V_stall = sqrt( (2 * W) / (rho * WingArea * CL_max) )
    float velocity_stall = sqrtf((2.0f * WEIGHT) / (RHO * WING_AREA * CL_MAX));
    
    while (1)
    {
        // Read both ADC channels
        int raw_ch0 = *adc_ch0_ptr;  // Potentially "velocity" pot
        int raw_ch1 = *adc_ch1_ptr;  // Potentially "AoA" pot
        
        // Check if both conversions have completed
        if ((raw_ch0 & 0x10000) && (raw_ch1 & 0x10000))
        {
            // Extract the 12-bit ADC values (bits [11:0])
            int adc_val0 = raw_ch0 & 0xFFF;  // Velocity pot raw
            int adc_val1 = raw_ch1 & 0xFFF;  // AoA pot raw
            
            // ----- Convert ADC values to physical units -----
            // Velocity: 0..4095 => 0..300 m/s
            float current_velocity = (adc_val0 / 4095.0f) * 300.0f;
            
            // AoA: (adc_val1/4095 - 0.5) * 60 => range of -30..+30 degrees
            float current_AoA = ((adc_val1 / 4095.0f) - 0.5f) * 60.0f;
            
            // ----- Stall Logic -----
            // If velocity < V_stall OR AoA > MAX_AOA => stall
            int stalling = (current_velocity < velocity_stall) || (current_AoA > MAX_AOA);
            
            // ----- Drive LEDs -----
            if (stalling)
            {
                // Turn on all 10 LEDs (bits 0..9 => 0x3FF)
                *jp1_data_ptr = 0x3FF;
            }
            else
            {
                // Turn off all LEDs
                *jp1_data_ptr = 0x000;
            }
        }
        
        // Delay to slow the loop
        delay();
    }
    
    return 0; // Never reached
}
