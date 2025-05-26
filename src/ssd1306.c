/**
 * @brief implementação biblioteca SSD1306
 */

#include "ssd1306.h"
#include "font.h"

void ssd1306_init(ssd1306_t *ssd, uint8_t width, uint8_t height, bool external_vcc, uint8_t address, i2c_inst_t *i2c) {
  ssd->width = width;
  ssd->height = height;
  ssd->pages = height / 8U;
  ssd->address = address;
  ssd->i2c_port = i2c;
  ssd->bufsize = ssd->pages * ssd->width + 1;
  ssd->ram_buffer = calloc(ssd->bufsize, sizeof(uint8_t));
  ssd->ram_buffer[0] = 0x40;
  ssd->port_buffer[0] = 0x80;
}

void ssd1306_config(ssd1306_t *ssd) {
  ssd1306_command(ssd, SET_DISP | 0x00);
  ssd1306_command(ssd, SET_MEM_ADDR);
  ssd1306_command(ssd, 0x01);
  ssd1306_command(ssd, SET_DISP_START_LINE | 0x00);
  ssd1306_command(ssd, SET_SEG_REMAP | 0x01);
  ssd1306_command(ssd, SET_MUX_RATIO);
  ssd1306_command(ssd, HEIGHT - 1);
  ssd1306_command(ssd, SET_COM_OUT_DIR | 0x08);
  ssd1306_command(ssd, SET_DISP_OFFSET);
  ssd1306_command(ssd, 0x00);
  ssd1306_command(ssd, SET_COM_PIN_CFG);
  ssd1306_command(ssd, 0x12);
  ssd1306_command(ssd, SET_DISP_CLK_DIV);
  ssd1306_command(ssd, 0x80);
  ssd1306_command(ssd, SET_PRECHARGE);
  ssd1306_command(ssd, 0xF1);
  ssd1306_command(ssd, SET_VCOM_DESEL);
  ssd1306_command(ssd, 0x30);
  ssd1306_command(ssd, SET_CONTRAST);
  ssd1306_command(ssd, 0xFF);
  ssd1306_command(ssd, SET_ENTIRE_ON);
  ssd1306_command(ssd, SET_NORM_INV);
  ssd1306_command(ssd, SET_CHARGE_PUMP);
  ssd1306_command(ssd, 0x14);
  ssd1306_command(ssd, SET_DISP | 0x01);
}

void ssd1306_command(ssd1306_t *ssd, uint8_t command) {
  ssd->port_buffer[1] = command;
  i2c_write_blocking(
    ssd->i2c_port,
    ssd->address,
    ssd->port_buffer,
    2,
    false
  );
}

void ssd1306_send_data(ssd1306_t *ssd) {
  ssd1306_command(ssd, SET_COL_ADDR);
  ssd1306_command(ssd, 0);
  ssd1306_command(ssd, ssd->width - 1);
  ssd1306_command(ssd, SET_PAGE_ADDR);
  ssd1306_command(ssd, 0);
  ssd1306_command(ssd, ssd->pages - 1);
  i2c_write_blocking(
    ssd->i2c_port,
    ssd->address,
    ssd->ram_buffer,
    ssd->bufsize,
    false
  );
}

void ssd1306_pixel(ssd1306_t *ssd, uint8_t x, uint8_t y, bool value) {
  uint16_t index = (y >> 3) + (x << 3) + 1;
  uint8_t pixel = (y & 0b111);
  if (value)
    ssd->ram_buffer[index] |= (1 << pixel);
  else
    ssd->ram_buffer[index] &= ~(1 << pixel);
}

/*
void ssd1306_fill(ssd1306_t *ssd, bool value) {
  uint8_t byte = value ? 0xFF : 0x00;
  for (uint8_t i = 1; i < ssd->bufsize; ++i)
    ssd->ram_buffer[i] = byte;
}*/

void ssd1306_fill(ssd1306_t *ssd, bool value) {
    // Itera por todas as posições do display
    for (uint8_t y = 0; y < ssd->height; ++y) {
        for (uint8_t x = 0; x < ssd->width; ++x) {
            ssd1306_pixel(ssd, x, y, value);
        }
    }
}


void ssd1306_rect(ssd1306_t *ssd, uint8_t top, uint8_t left, uint8_t width, uint8_t height, bool value, bool fill) {
  for (uint8_t x = left; x < left + width; ++x) {
    ssd1306_pixel(ssd, x, top, value);
    ssd1306_pixel(ssd, x, top + height - 1, value);
  }
  for (uint8_t y = top; y < top + height; ++y) {
    ssd1306_pixel(ssd, left, y, value);
    ssd1306_pixel(ssd, left + width - 1, y, value);
  }

  if (fill) {
    for (uint8_t x = left + 1; x < left + width - 1; ++x) {
      for (uint8_t y = top + 1; y < top + height - 1; ++y) {
        ssd1306_pixel(ssd, x, y, value);
      }
    }
  }
}

void ssd1306_line(ssd1306_t *ssd, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool value) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);

    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    int err = dx - dy;

    while (true) {
        ssd1306_pixel(ssd, x0, y0, value); // Desenha o pixel atual

        if (x0 == x1 && y0 == y1) break; // Termina quando alcança o ponto final

        int e2 = err * 2;

        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}


void ssd1306_hline(ssd1306_t *ssd, uint8_t x0, uint8_t x1, uint8_t y, bool value) {
  for (uint8_t x = x0; x <= x1; ++x)
    ssd1306_pixel(ssd, x, y, value);
}

void ssd1306_vline(ssd1306_t *ssd, uint8_t x, uint8_t y0, uint8_t y1, bool value) {
  for (uint8_t y = y0; y <= y1; ++y)
    ssd1306_pixel(ssd, x, y, value);
}

// Função para desenhar um caractere
void ssd1306_draw_char(ssd1306_t *ssd, char c, uint8_t x, uint8_t y)
{
  uint16_t index = 0;

  if (c == ' ')
  {
    index = 0; // Para espaços vazios
  }
  else if (c >= 'A' && c <= 'Z')
  {
    index = (c - 'A' + 11) * 8; // Para letras maiúsculas
  }
  else if (c >= '0' && c <= '9')
  {
    index = (c - '0' + 1) * 8; // Para números
  }
  else if (c >= 'a' && c <= 'z')
  {
    index = (c - 'a' + 37) * 8; // Para letras minúsculas (ajuste o deslocamento corretamente)
  }
  else if (c == '.')
  {
    // Desenhando um ponto 5x5 e mais baixo
    for (uint8_t i = 0; i < 5; ++i) {
      for (uint8_t j = 0; j < 5; ++j) {
        if (i == 2 && j == 2) {  // Coloca o ponto no centro (mais baixo)
          ssd1306_pixel(ssd, x + i, y + j + 3, 1);  // Ajuste da altura para torná-lo mais baixo
        }
      }
    }
    return; // Após desenhar o ponto, não é necessário desenhar mais nada
  }
  else if (c == ',')
  {
    index = 64 * 8; // Supondo que a vírgula seja o próximo caractere
  }
  else if (c == '?')
  {
    index = 65 * 8; // Supondo que o ponto de interrogação seja o seguinte
  }
  else
  {
    return; // Se o caractere não for suportado, sai da função
  }

  // Desenha outros caracteres normalmente
  for (uint8_t i = 0; i < 8; ++i)
  {
    uint8_t line = font[index + i];
    for (uint8_t j = 0; j < 8; ++j)
    {
      ssd1306_pixel(ssd, x + i, y + j, line & (1 << j));
    }
  }
}

// Função para desenhar uma string
void ssd1306_draw_string(ssd1306_t *ssd, const char *str, uint8_t x, uint8_t y)
{
  while (*str)
  {
    ssd1306_draw_char(ssd, *str++, x, y);
    x += 8;
    if (x + 8 >= ssd->width)
    {
      x = 0;
      y += 8;
    }
    if (y + 8 >= ssd->height)
    {
      break;
    }
  }
}

/**
 * Configura o display I2C
 */
void config_i2c_display(ssd1306_t *ssd)
{
  i2c_init(I2C_PORT, 400 * 1000); //Inicializa I2C a 400 KHz
  
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA); // Pull up the data line
  gpio_pull_up(I2C_SCL); // Pull up the clock line

  ssd1306_init(ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(ssd); // Configura o display
  ssd1306_send_data(ssd); // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(ssd, false);
  ssd1306_send_data(ssd);
}


/**
 * @brief desenhar uma imagem com base em um array
 */
void ssd1306_draw_image(ssd1306_t *ssd, uint8_t _imagem_data[1024]) 
{
    uint8_t width = 128;
    uint8_t height = 64;
    uint8_t bytes_per_row = width / 8;  // 128px de largura -> 16 bytes por linha

    uint16_t i = 0;
    for (uint8_t y = 0; y < height; y++)  // Percorre linha por linha
    {
        for (uint8_t x_byte = 0; x_byte < bytes_per_row; x_byte++)  // Percorre os bytes na horizontal
        {
            uint8_t byte = _imagem_data[i++];  // Pega o byte correspondente
            for (uint8_t bit = 0; bit < 8; bit++)  // Percorre os bits dentro do byte
            {
                bool pixel_val = (byte >> (7 - bit)) & 0x01;  // MSB-first
                ssd1306_pixel(ssd, (x_byte * 8) + bit, y, pixel_val); // Define o pixel na tela
            }
        }
    }
    ssd1306_send_data(ssd);  // Atualiza a tela com os novos pixels
}