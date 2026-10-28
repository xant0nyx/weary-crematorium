//Sesion 7 ELO312 3_4_Preg + BONUS
#include "display.h"
#include "lsm303agr.h"
#include <stdio.h>        
#include <string.h>


/* USER CODE BEGIN PV */
volatile uint8_t flag_100ms = 0;
volatile uint8_t flag_uart = 0;
char rx_buffer[20];
volatile uint8_t rx_index = 0;
uint8_t rx_byte;

// Variables de medición
float mag_x = 0.0, mag_y = 0.0, mag_z = 0.0;
uint8_t lcd_counter = 0;

// Variables para el Comando B[cite: 25]
uint8_t recolectando_B = 0;
uint16_t muestras_B = 0;
float buffer_mx[1000], buffer_my[1000], buffer_mz[1000];
/* USER CODE END PV */


/* USER CODE BEGIN WHILE */
  while (1)
  {
      // ==================================================================
      // 1. TAREA DEL TEMPORIZADOR (Cada 100 ms)[cite: 24]
      // ==================================================================
      if (flag_100ms) {
          flag_100ms = 0;

          /*A)LECTURA DEL SENSOR LSM303AGR */
          int16_t data_raw_magnetic[3];
          
          // Función estándar de la librería lsm303agr_reg.c para leer los datos crudos
          lsm303agr_mag_magnetic_raw_get(&dev_ctx_mag, data_raw_magnetic);
          
          // Función de la librería para convertir a Gauss (o milli-Gauss)
          mag_x = lsm303agr_m_from_lsb_to_mgauss(data_raw_magnetic[0]);
          mag_y = lsm303agr_m_from_lsb_to_mgauss(data_raw_magnetic[1]);
          mag_z = lsm303agr_m_from_lsb_to_mgauss(data_raw_magnetic[2]);

          /* --- B) MODO COMANDO 'B' (Registrar 1000 mediciones) */
          if (recolectando_B) {
              buffer_mx[muestras_B] = mag_x;
              buffer_my[muestras_B] = mag_y;
              buffer_mz[muestras_B] = mag_z;
              muestras_B++;

              if (muestras_B >= 1000) {
                  recolectando_B = 0; // Termina la recolección
                  
                  // Enviar el CSV con las 1000 mediciones por UART
                  for (int i = 0; i < 1000; i++) {
                      printf("%.2f,%.2f,%.2f\n", buffer_mx[i], buffer_my[i], buffer_mz[i]);
                  }
              }
          }

          /* --- C) ACTUALIZACIÓN DEL DISPLAY (Cada 1 segundo) */
          lcd_counter++;
          if (lcd_counter >= 10) { // 10 iteraciones de 100ms = 1 segundo
              lcd_counter = 0;
              
              // Asumiendo que __io_putchar está redirigido a display_putchar()
              display_clear();
              display_set_pos(display_lcd_addr(0));
              printf("mx:%.2f my:%.2f", mag_x, mag_y);
              display_set_pos(display_lcd_addr(1));
              printf("mz:%.2f", mag_z);
          }
      }

      // ==================================================================
      // 2. TAREA DE LA UART (Procesamiento de Comandos)[cite: 25]
      // ==================================================================
      if (flag_uart) {
          flag_uart = 0;
          
          // Aseguramos que la cadena esté cerrada para poder usar strcmp()
          rx_buffer[rx_index] = '\0'; 
          
          // La guía exige detectar los comandos seguidos de \n[cite: 25]
          if (strcmp(rx_buffer, "A\n") == 0) {
              // Comando A: Responde con la última medición[cite: 25]
              printf("mx:%.2f my:%.2f mz:%.2f\n", mag_x, mag_y, mag_z);
          } 
          else if (strcmp(rx_buffer, "B\n") == 0) {
              // Comando B: Inicia registro de 1000 mediciones (100 segundos)[cite: 25]
              recolectando_B = 1;
              muestras_B = 0;
          } 
          else {
              // Comando Desconocido[cite: 25]
              printf("Comando desconocido\n");
          }
          
          // Limpiar el buffer para el siguiente comando
          rx_index = 0;
          memset((void*)rx_buffer, 0, sizeof(rx_buffer));
      }
  }
  /* USER CODE END WHILE */



/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM3) { // Asumiendo TIM3 configurado a 100ms[cite: 24]
        flag_100ms = 1;
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        if (rx_byte == '\n') {
            rx_buffer[rx_index] = '\n'; // Guardar el salto de línea exigido por la guía[cite: 25]
            rx_index++;
            flag_uart = 1;              // Levantar bandera para procesar en el while(1)
        } else {
            if (rx_index < 19) {
                rx_buffer[rx_index] = rx_byte;
                rx_index++;
            }
        }
        // Rehabilitar interrupción
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
    }
}

// Display LCD
int __io_putchar(int ch) {
    return display_putchar(ch);
}
/* USER CODE END 4 */