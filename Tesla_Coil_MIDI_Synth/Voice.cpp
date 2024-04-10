#include "Voice.h"

#include <Arduino.h>

namespace Voice {

Voice voices[NVOICES];
volatile uint8_t voicesUpdating = 0;

void initVoices() {
  memset(voices, 0, sizeof(voices));
  
  // Enable watchdog timer so we get reset after 1s if we crash
  // Doesn't seem to work since maybe Arduino stuff writes to it first... whatever
  WDT->WDT_MR = WDT_MR_WDRSTEN | WDT_MR_WDV(255) | WDT_MR_WDD(255);

  PMC->PMC_WPMR = 0x504D4300; // Disable write protect
  PMC->PMC_PCER0 = (1<<ID_TC0) | (1<<ID_TC1) | (1<<ID_TC2) | (1<<ID_TC3) | (1<<ID_TC4); // Enable clock to timers
  PMC->PMC_PCER1 = (1<<(ID_TC5-32));

  // Init timers
  for(unsigned int x=0; x<NVOICES; x++) {
    if(voiceConfigs[x].peripheralab) voiceConfigs[x].port->PIO_ABSR |= (1<<voiceConfigs[x].portN); // Select peripheral B
    else voiceConfigs[x].port->PIO_ABSR &= ~(1<<voiceConfigs[x].portN); // Select peripheral A
    voiceConfigs[x].port->PIO_PDR = (1<<voiceConfigs[x].portN); // Disable PIO control, allow peripheral to use it
    
    voiceConfigs[x].timer->TC_WPMR = 0x54494D00; // Disable write protect
    voiceConfigs[x].channel->TC_CCR = TC_CCR_CLKDIS; // Disable clock
    voiceConfigs[x].channel->TC_IDR = 0xFF; // Disable interrupts
    voiceConfigs[x].channel->TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK1 // MCK/2
      | TC_CMR_EEVT_XC0 // Make sure EEVT thing is set so TIOB can be an output
      | TC_CMR_WAVSEL_UP_RC // Select up counting with reset at RC compare
      | TC_CMR_WAVE; // Waveform mode
      
    voiceConfigs[x].channel->TC_RC = F_CPU/2/100; // 100Hz
    
    if(voiceConfigs[x].timerab) {
      voiceConfigs[x].channel->TC_CMR |= TC_CMR_BCPB_CLEAR // Clear TIOB on compare with RB
        | TC_CMR_BSWTRG_CLEAR; // Clear TIOB on software trigger
      voiceConfigs[x].channel->TC_RB = 1;
    } else {
      voiceConfigs[x].channel->TC_CMR |= TC_CMR_ACPA_CLEAR // Clear TIOA on compare with RA
        | TC_CMR_ASWTRG_CLEAR; // Clear TIOA on software trigger
      voiceConfigs[x].channel->TC_RA = 1;
    }
    
    voiceConfigs[x].channel->TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG; // Enable clock
  }
}

}
