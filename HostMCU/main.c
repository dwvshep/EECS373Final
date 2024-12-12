/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "stdio.h"
#include <stdlib.h>
#include "serialDisplay.h"
#include "arm_math.h"
#include <math.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define AUDIO_BUFFER_SIZE 1024  // Size of the buffer to store audio samples
#define FFT_OUTPUT_SIZE 256
#define SAMPLE_RATE 44100
#define NUM_BINS 15
uint16_t audio_buffer[AUDIO_BUFFER_SIZE];  // Buffer to store ADC samples
float fft_input[AUDIO_BUFFER_SIZE / 2];          // Buffer for FFT input (as float)
float fft_output[FFT_OUTPUT_SIZE];         // Buffer for FFT output (frequency domain)
short bpm_buffer[AUDIO_BUFFER_SIZE / 2];
float log_scale_output[NUM_BINS];
float log_scale_norms[NUM_BINS];

volatile uint8_t buffer_half_full = 0;       // Flag for half buffer full
volatile uint8_t buffer_full = 0;            // Flag for full buffer
static float *window;      /* FFT window */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
        buffer_half_full = 1;  // First half of buffer is filled
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
        buffer_full = 1;  // Second half of buffer is filled
    }
}
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;

/* USER CODE BEGIN PV */

SongPacket song;
volatile char oldGenre[1] = "Z";

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_TIM6_Init(void);
static void MX_TIM7_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */
uint8_t spi_buf[FFT_OUTPUT_SIZE - 2];

void ParseUSARTMessage(char *buffer);
#define BUFFER_SIZE 256
uint8_t buffer[BUFFER_SIZE];
volatile uint16_t buffer_pos = 0;
uint8_t byte_received = 0;


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3) {
    	//printf("RX_UART3\n");
        if (byte_received == '$') {
            buffer[buffer_pos] = '\0';  // Null-terminate the received string
            buffer_pos = 0;  // Reset buffer position

            ParseUSARTMessage((char *)buffer); // Call the parsing logic

            // Send acknowledgment back to Python
            char ack_message[] = "ACK: Message Received\n";
            HAL_UART_Transmit(&huart3, (uint8_t *)ack_message, strlen(ack_message), HAL_MAX_DELAY);
        } else {
            if (buffer_pos < BUFFER_SIZE - 1) {
                buffer[buffer_pos++] = byte_received;  // Store received byte
            } else {
                // Handle buffer overflow
                printf("Error: Buffer overflow\n");
                buffer_pos = 0;  // Reset buffer to prevent undefined behavior

                // Send error message back to Python
                char error_message[] = "ERROR: Buffer Overflow\n";
                HAL_UART_Transmit(&huart3, (uint8_t *)error_message, strlen(error_message), HAL_MAX_DELAY);
            }
        }

        // Re-enable interrupt for the next byte
        HAL_UART_Receive_IT(&huart3, &byte_received, 1);
    } else if (huart->Instance == USART1) {
    	//printf("RX_UART1\n");
    }

}


void ParseUSARTMessage(char *buffer) {
    static int song_name_length = 0;
    static char *song_name = NULL;

    static int artists_length = 0;
    static char *artists = NULL;

    static int progress = 0;
    static int duration = 0;

    char temp[100];
    int temp_pos = 0;

    // Deactivate Current Song:
    song.activeSong = 0;
    song.activePrint = 0;

    for (int i = 0; buffer[i] != '\0'; i++) {
        switch (buffer[i]) {
            case '#':
                temp[temp_pos] = '\0';
                song_name_length = atoi(temp);
                temp_pos = 0;
                song_name = malloc(song_name_length + 1);
                break;

            case '@':
                strncpy(song_name, temp, song_name_length);
                song_name[song_name_length] = '\0';
                temp_pos = 0;
                break;

            case '^':
                temp[temp_pos] = '\0';
                artists_length = atoi(temp);
                temp_pos = 0;
                artists = malloc(artists_length + 1);
                break;

            case '*':
                temp[temp_pos] = '\0';
                strncpy(artists, temp, artists_length);
                artists[artists_length] = '\0';
                temp_pos = 0;
                break;

            case '!':
                temp[temp_pos] = '\0';
                progress = atoi(temp);
                temp_pos = 0;
                break;

            default:
                temp[temp_pos++] = buffer[i];
                break;
        }
    }

    temp[temp_pos] = '\0';
    duration = atoi(temp);

    // Debugging output
    printf("Song Name: %s (Length: %d)\r\n", song_name, song_name_length);
    printf("Artists: %s (Length: %d)\r\n", artists, artists_length);
    printf("Progress: %d\r\n", progress);
    printf("Duration: %d\r\n", duration);

    // Assign to song structure
    song.nameLength = song_name_length;
    song.songName = (uint8_t *)song_name;
    song.artistName = (uint8_t *)artists;
    song.artistLength = artists_length;
    song.progress = progress;
    song.duration = duration;
    song.paused = 0;
    song.activeSong = 1;

}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

	if (htim->Instance == TIM6) {
	        // Your code here
	        printf("TIM6 Interrupt Triggered\n");
	    }

	return;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void compute_log_scale_bins(const float *fft_output, float *log_scale_output) {
    float freq_resolution = (float)SAMPLE_RATE / FFT_OUTPUT_SIZE; // Frequency resolution (Hz per bin)
    //float freq_min = 1;    // Minimum frequency to avoid log(0)
    //float freq_max = (float)SAMPLE_RATE / 2; // Nyquist frequency
    float log_bins[NUM_BINS + 1];
    int i, bin_start, bin_end;

    // Generate logarithmic scale boundaries
    log_bins[0] = 1;
    log_bins[1] = 176;
    log_bins[2] = 352;
    log_bins[3] = 528;
    log_bins[4] = 708;
    log_bins[5] = 880;
    log_bins[6] = 1056;
    log_bins[7] = 1232;
    log_bins[8] = 1408;
    log_bins[9] = 1760;
    log_bins[10] = 2500;
    log_bins[11] = 5000;
    log_bins[12] = 7500;
    log_bins[13] = 10000;
    log_bins[14] = 15000;
    log_bins[15] = 20000;

    // Initialize the log scale output bins
    for (i = 0; i < NUM_BINS; i++) {
        log_scale_output[i] = 0.0f;
    }

    // Combine FFT bins into the log-scale bins
    for (i = 0; i < NUM_BINS; i++) {
        bin_start = (int)(log_bins[i] / freq_resolution);
        bin_end = (int)(log_bins[i + 1] / freq_resolution);
        if (bin_end > FFT_OUTPUT_SIZE / 2) {
            bin_end = FFT_OUTPUT_SIZE / 2; // Ensure bin_end doesn't exceed half of the FFT size
        }

        // Sum magnitudes within the current range
        for (int j = bin_start; j < bin_end; j++) {
            log_scale_output[i] += fft_output[j];
            //log_scale_output[i] = log_scale_output[i] / ((float)(bin_end - bin_start));
        }
        log_scale_output[0] -= 2.05;
        log_scale_output[1] -= 0.4;
        log_scale_output[2] -= 0.1;
        //log_scale_output[9] = log_scale_output[9] / (float)(2.0);
    }
}

void send_fft(const float *array, size_t size){
	for(size_t i = 0; i < size / 2 - 1; i++){
		spi_buf[2 * i] = (uint8_t)(i);
		spi_buf[2 * i + 1] = (uint8_t)(array[i]);
	}
	uint16_t num_bands = FFT_OUTPUT_SIZE / 2 - 1;
	num_bands = 15;
	for(size_t j = 0; j < num_bands; j++){
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
		HAL_SPI_Transmit(&hspi1, (uint8_t*)&spi_buf[2 * j], 2, HAL_MAX_DELAY);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
	}
}

void compute_fft(float* input, float* output, uint16_t size) {
    arm_rfft_fast_instance_f32 S;  // FFT instance

    // Initialize the FFT instance for the specified FFT size
    arm_rfft_fast_init_f32(&S, size);

    // Perform the FFT
    arm_rfft_fast_f32(&S, input, output, 0);


    // Calculate the magnitude of each frequency bin
    //output[0] = output[0];
    for (int i = 1; i < size / 2; i++) {
        output[i - 1] = floor(sqrtf(output[2 * i] * output[2 * i] + output[2 * i + 1] * output[2 * i + 1]) * 64);
    }
    compute_log_scale_bins(output, log_scale_output);

}

/////////////////////////////////////////////////////////////////////////////////
////////////////////////////// Timer Interrupts: ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

// Handles Updating the Progress Bar and Time Display:
void TIM6_DAC_IRQHandler(void) {
  /* USER CODE BEGIN TIM6_DAC_IRQn 0 */

  /* USER CODE END TIM6_DAC_IRQn 0 */
  HAL_TIM_IRQHandler(&htim6);
  /* USER CODE BEGIN TIM6_DAC_IRQn 1 */

  // Ensure Active Song + Artist Print (Added):
  if (!song.activeSong && !song.activePrint) {
	  return;
  }

  // Ensure Song isn't Paused (Added):
  if (song.paused) {
	  return;
  }

  // Update Time (Added):
  updateTime(&song);

  // Display Song:
  displayTime(&huart1, &song);

  //updateTime(&song);

  // Update Bar:
  updateProgress(&huart1, &song);

  /* USER CODE END TIM6_DAC_IRQn 1 */
}

// Handles Displaying the Song Title:
void TIM7_IRQHandler(void){
  /* USER CODE BEGIN TIM7_IRQn 0 */

  /* USER CODE END TIM7_IRQn 0 */
  HAL_TIM_IRQHandler(&htim7);
  /* USER CODE BEGIN TIM7_IRQn 1 */

  // Ensure Active Song + Artist Print (Added):
  if (!song.activeSong || !song.activePrint) {
  	return;
  }

  // Scoll Position:
  static volatile int scrollPos = 20;

  // Handle Position on Interrupt:
  if (scrollPos > -(song.nameLength)) {
  	scrollPos--;
  } else {
  	scrollPos = 20;
  }

  char displayBuffer[21];
  memset(displayBuffer, ' ', 20);
  displayBuffer[20] = '\0';

  // Calculate Position:
  for (int i = 0; i < song.nameLength; i++) {
  	if (scrollPos + i < 20 && scrollPos + i >= 0) {
  		displayBuffer[scrollPos + i] = song.songName[i];
  	}
  }

  // Update Buffer:
  setCursorPos(&huart1, 0x00);
  HAL_UART_Transmit(&huart1, (uint8_t*)displayBuffer, 20, 1000);

  /* USER CODE END TIM7_IRQn 1 */
}

/////////////////////////////////////////////////////////////////////////////////
////////////////////////////// Button Interrupts: ///////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

//void EXTI15_10_IRQHandler(void){
//  /* USER CODE BEGIN EXTI15_10_IRQn 0 */
//
//  /* USER CODE END EXTI15_10_IRQn 0 */
//  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
//  /* USER CODE BEGIN EXTI15_10_IRQn 1 */
//  char message[] = "CHRI\n";
//  song.genre = "GROOVY";
//
//   //Send message over UART
//  HAL_UART_Transmit(&huart3, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);
//  /* USER CODE END EXTI15_10_IRQn 1 */
//}


// Handles the Pause/Play Button:
void EXTI1_IRQHandler(void) {
  /* USER CODE BEGIN EXTI1_IRQn 0 */

  // Reset Flag:
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);



  // Ensure Song is Active:
  if (!song.activeSong) {
	  return;
  }


  //static volatile int paused = 0;

  char pause[] = "PAUSE\n";
  char play[] = "PLAY\n";

  char pauseStr[] = "Song is Paused";
  char playStr[] = "Song is Playing";

  // Song is Playing -> Pause:
  if (!song.paused) {

	  // Transmit to Spotify:
	  HAL_UART_Transmit(&huart3, (uint8_t *)pause, strlen(pause), HAL_MAX_DELAY);
	  char genre[]= "S";
	  HAL_UART_Transmit(&huart2, &genre, 1, HAL_MAX_DELAY);

	  // Screen Message Debug:
//	  clearDisplay(&huart1);
//	  HAL_UART_Transmit(&huart1, (uint8_t *)pauseStr, 14, HAL_MAX_DELAY);

	  // Change Song Variables + Flag:
	  song.paused = 1;
	  //paused = 1;

	  return;
  }

  // Song is Paused -> Play:

  // Transmit to Spotify:
  HAL_UART_Transmit(&huart3, (uint8_t *)play, strlen(play), HAL_MAX_DELAY);
  HAL_UART_Transmit(&huart2, &oldGenre, 1, HAL_MAX_DELAY);



  // Screen Message Debug:
//  clearDisplay(&huart1);
//  HAL_UART_Transmit(&huart1, (uint8_t *)playStr, 15, HAL_MAX_DELAY);



  // Change Song Variables + Flag:
  song.paused = 0;
  //paused = 0;

  return;
  /* USER CODE END EXTI1_IRQn 0 */
}

void EXTI3_IRQHandler(void) {
  /* USER CODE BEGIN EXTI3_IRQn 0 */
	char message[] = "JAZZ\n";
	song.paused = 0;
	HAL_UART_Transmit(&huart3, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);

	char genre[]= "J";
	oldGenre[0] = 'J';
	HAL_UART_Transmit(&huart2, &genre, 1, HAL_MAX_DELAY);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);

  /* USER CODE END EXTI3_IRQn 0 */
  /* USER CODE BEGIN EXTI3_IRQn 1 */

  /* USER CODE END EXTI3_IRQn 1 */
}

void EXTI4_IRQHandler(void) {
  /* USER CODE BEGIN EXTI4_IRQn 0 */
	char message[] = "CHRI\n";
	song.paused = 0;
	HAL_UART_Transmit(&huart3, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);
	char genre[]= "C";
	oldGenre[0] = 'C';
	HAL_UART_Transmit(&huart2, &genre, 1, HAL_MAX_DELAY);

  /* USER CODE END EXTI4_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
  /* USER CODE BEGIN EXTI4_IRQn 1 */

  /* USER CODE END EXTI4_IRQn 1 */
}


void EXTI0_IRQHandler(void) {
  /* USER CODE BEGIN EXTI0_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);

  if (!song.activeSong) {
	  return;
  }

  //song.activeSong = 0;
  char skip[] = "NEXT\n";
  song.paused = 0;
  HAL_UART_Transmit(&huart3, (uint8_t *)skip, strlen(skip), HAL_MAX_DELAY);

  /* USER CODE END EXTI0_IRQn 0 */
  /* USER CODE BEGIN EXTI0_IRQn 1 */
  HAL_UART_Transmit(&huart2, &oldGenre, 1, HAL_MAX_DELAY);

  /* USER CODE END EXTI0_IRQn 1 */
}


void EXTI15_10_IRQHandler(void) {
  /* USER CODE BEGIN EXTI15_10_IRQn 0 */

  char message[] = "ROCK\n";
  song.paused = 0;
  HAL_UART_Transmit(&huart3, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);
  char genre[]= "R";
  oldGenre[0] = 'R';
  HAL_UART_Transmit(&huart2, &genre, 1, HAL_MAX_DELAY);

  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);

  /* USER CODE END EXTI15_10_IRQn 0 */
  /* USER CODE BEGIN EXTI15_10_IRQn 1 */

  /* USER CODE END EXTI15_10_IRQn 1 */
}

void EXTI9_5_IRQHandler(void){
  /* USER CODE BEGIN EXTI9_5_IRQn 0 */

  /* USER CODE END EXTI9_5_IRQn 0 */
  char message[] = "GROV\n";
  song.paused = 0;
  HAL_UART_Transmit(&huart3, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);
  char genre[]= "G";
  oldGenre[0] = 'G';
  HAL_UART_Transmit(&huart2, &genre, 1, HAL_MAX_DELAY);

  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);


  /* USER CODE BEGIN EXTI9_5_IRQn 1 */

  /* USER CODE END EXTI9_5_IRQn 1 */
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  // Initialize Song Packet:
  song.activeSong = 0;
  song.activePrint = 0;
  song.inactivePrint = 0;
  song.paused = 0;
  song.duration = 0;
  song.progress = 0;

  window = (float *)malloc(AUDIO_BUFFER_SIZE / 2 * sizeof(float));
  for (int i = 0; i < AUDIO_BUFFER_SIZE / 2; i++) {
	  window[i] = (float)(0.54 - 0.46 * (sin((2 * PI * i) / (AUDIO_BUFFER_SIZE / 2 - 1.0))));
  }

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART3_UART_Init();
  MX_LPUART1_UART_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start_IT(&htim6);
  HAL_TIM_Base_Start_IT(&htim7);



  HAL_UART_Receive_IT(&huart3, &byte_received, 1);

  clearDisplay(&huart1);
  HAL_Delay(100);

  //HAL_UART_Transmit(&huart2, song.genre, strlen((const char *)song.genre), HAL_MAX_DELAY);

  //HAL_UART_Transmit(&huart2, (uint8_t *)song.genre, strlen(song.genre), HAL_MAX_DELAY);

  printf("STM32 UART Interface Initialized!\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  // Start ADC in DMA mode
  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)audio_buffer, AUDIO_BUFFER_SIZE) != HAL_OK) {
    // ADC conversion start error
    Error_Handler();
  }

  // Enable global interrupts
  //__enable_irq();

  char genre[]= "A";
  HAL_UART_Transmit(&huart2, &genre, 1, HAL_MAX_DELAY);
  HAL_Delay(8000);
//  genre[0]= 'J';
//  HAL_UART_Transmit(&huart2, &genre, 1, HAL_MAX_DELAY);
////  HAL_Delay(5000);
////  genre[0]= 'R';
////  HAL_UART_Transmit(&huart2, &genre, 1, HAL_MAX_DELAY);
////  HAL_Delay(7000);
////  genre[0]= 'J';
////  HAL_UART_Transmit(&huart2, &genre, 1, HAL_MAX_DELAY);
//// HAL_Delay(7000);
//// genre[0]= 'G';
//// HAL_UART_Transmit(&huart2, &genre, 1, HAL_MAX_DELAY);
//  HAL_Delay(7000);
//  genre[0]= 'S';
//  HAL_UART_Transmit(&huart2, &genre, 1, HAL_MAX_DELAY);
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  displaySong(&huart1, &song);
	  if (buffer_half_full) {
	  		  // Copy first half of audio_buffer to fft_input
	  	      for (int i = 0; i < AUDIO_BUFFER_SIZE / 2; i++) {
	  	          fft_input[i] = window[i] * ((float)audio_buffer[i] - 2048) / 4096;  // Convert to float if necessary
	  	          bpm_buffer[i] = ((short)audio_buffer[i] - 2048);
	  	      }

	  	      // Perform FFT on the first half of the buffer
	  	      compute_fft(fft_input, fft_output, FFT_OUTPUT_SIZE);

	  	      buffer_half_full = 0;  // Reset the flag after processing

	  	      send_fft(log_scale_output, 64);

	  	      //printArray(fft_output, FFT_OUTPUT_SIZE / 2);
	  	      //printf("half: %d\n", cnt);
	  	      //cnt = cnt + 1;

	  	      // Send to BPM detection
	  	      /*
	  	      if (BPM_put_samples(bpm_buffer, AUDIO_BUFFER_SIZE / 2) == 1) {
	  	          uint32_t bpm = BPM_get_bpm();
	  	          printf("BPM Detected: %lu\n", (unsigned long)bpm);
	  	          return 0;
	  	      }
	  	      else{
	  	    	  //printf("BPM Not Detected\n");
	  	      }
	  		*/

	  	  }

	  	  if (buffer_full) {
	  	      // Copy second half of audio_buffer to fft_input
	  	      for (int i = AUDIO_BUFFER_SIZE / 2; i < AUDIO_BUFFER_SIZE; i++) {
	  	          fft_input[i - AUDIO_BUFFER_SIZE / 2] = window[i - AUDIO_BUFFER_SIZE / 2] * ((float)audio_buffer[i] - 2048) / 4096;
	  	          bpm_buffer[i - AUDIO_BUFFER_SIZE / 2] = ((short)audio_buffer[i] - 2048);
	  	      }

	  	      // Perform FFT on the second half of the buffer
	  	      compute_fft(fft_input, fft_output, FFT_OUTPUT_SIZE);

	  	      buffer_full = 0;  // Reset the flag after processing

	  	      send_fft(log_scale_output, 64);

	  	      //printArray(fft_output, FFT_OUTPUT_SIZE / 2);
	  	      //printf("full: %d\n", cnt);
	  	      //cnt = cnt + 1;

	  	      /*
	  	      if (BPM_put_samples(bpm_buffer, AUDIO_BUFFER_SIZE / 2) == 1) {
	  	      	  uint32_t bpm = BPM_get_bpm();
	  	      	  printf("BPM Detected: %lu\n", (unsigned long)bpm);
	  	      	  return 0;
	  	      }
	  	      else{
	  	      	  //printf("BPM Not Detected\n");
	  	      }
	  		*/

	  	  }


//	  // Function Proto:
//	  if (song.activeSong == 0 && song.inactiveDisplay == 0) {
//		  char paused[] = "No Active Song";
//		  HAL_UART_Transmit(&huart1, &paused, strlen(paused), 20);
//		  song.inactiveDisplay = 1;
//	  } else if (song.activeSong == 1 && song.intialDisplay == 0) {
//		  clearDisplay(&huart1);
//		  HAL_UART_Transmit(&huart1, song.songName, song.nameLength, 20);
//		  song.intialDisplay = 1;
//	  }


  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV8;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_92CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 9600;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  hlpuart1.FifoMode = UART_FIFOMODE_DISABLE;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_LSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */
	__HAL_RCC_TIM6_CLK_ENABLE();

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 39999;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 99;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */
  HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief TIM7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */
	__HAL_RCC_TIM7_CLK_ENABLE();

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 3999;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 499;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */
  HAL_NVIC_SetPriority(TIM7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM7_IRQn);

  /* USER CODE END TIM7_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  HAL_PWREx_EnableVddIO2();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin : PE2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF13_SAI1;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PE3 PE4 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PF0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : PF1 PF2 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : PF7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF13_SAI1;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB2 PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PE7 PE8 PE9 PE10
                           PE11 PE12 PE13 */
  GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PE14 PE15 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF3_TIM1_COMP1;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB13 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF13_SAI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB14 */
  GPIO_InitStruct.Pin = GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF14_TIM15;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PC6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PC7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC8 PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA8 PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PD0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PD1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PD2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PD7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PB3 PB4 PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PE0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&hlpuart1, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
