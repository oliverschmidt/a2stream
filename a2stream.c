/******************************************************************************

Copyright (c) 2022, Oliver Schmidt
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL OLIVER SCHMIDT BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/

#include <fcntl.h>
#include <ctype.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <device.h>
#include <apple2_filetype.h>

#include <ip65.h>

#include "w5100.h"
#include "w5100_http.h"
#include "player.h"
#include "linenoise.h"

#include "a2stream.h"

#define write_main()  (*(uint8_t *)0xC004 = 0)
#define write_aux()   (*(uint8_t *)0xC005 = 0)
#define text_off()    (*(uint8_t *)0xC050 = 0)
#define text_on()     (*(uint8_t *)0xC051 = 0)
#define mix_off()     (*(uint8_t *)0xC052 = 0)
#define mix_on()      (*(uint8_t *)0xC053 = 0)
#define page_1()      (*(uint8_t *)0xC054 = 0)
#define page_2()      (*(uint8_t *)0xC055 = 0)
#define hires_off()   (*(uint8_t *)0xC056 = 0)
#define hires_on()    (*(uint8_t *)0xC057 = 0)
#define dhires()      (*(uint8_t *)0xC05E = 0)
#define shires()      (*(uint8_t *)0xC05F = 0)

static bool match(const char *filter, const char *string)
{
  while (*filter)
  {
    if (!*string)
    {
      return false;
    }
    if (toupper(*filter++) != toupper(*string++))
    {
      return false;
    }
  }
  return true;
}

static void completion(const char *line, linenoiseCompletions *lc)
{
  if (match(line, "http://"))
  {
    linenoiseAddCompletion(lc, "http://");
  }
  if (match(line, "http://www."))
  {
    linenoiseAddCompletion(lc, "http://www.");
  }
  if (isalnum(line[strlen(line) - 1]))
  {
    char *buffer = malloc(0x200);

    linenoiseAddCompletion(lc, strcat(strcpy(buffer, line), ".a2s"));
    linenoiseAddCompletion(lc, strcat(strcpy(buffer, line), ".a2stream"));

    free(buffer);
  }
}

static void error_exit(void)
{
  printf("- %s\n", ip65_strerror(ip65_error));
  exit(EXIT_FAILURE);
}

static void confirm_exit(void)
{
  printf("\nPress any key");
  cgetc();
}

bool load(register uint8_t *ptr, uint16_t len, bool aux)
{
  register uint16_t i = len;
  register volatile uint8_t *data = w5100_data;

  #ifdef HAVE_ETH
  while (w5100_receive_request() < len)
  {
    if (!w5100_connected() || input_check_for_abort_key())
    {
      return false;
    }
  }
  #endif

  if (aux)
  {
    write_aux();
  }

  // Only - actual - register (aka zero page) variables here !!!
  do
  {
    *ptr++ = *data;
  }
  while (--i);

  if (aux)
  {
    write_main();
  }

  w5100_receive_commit(len);
  return true;
}

bool load_hires(bool page2)
{
  uint8_t *ptr;
  bool ok;
  uint8_t x = wherex();

  for (ptr = (uint8_t *)0x2000; ptr < (uint8_t *)0x4000; ptr += 0x0800)
  {
    if (page2)
    {
      page_2();
    }

    ok = load(ptr, 0x0800, false);

    if (page2)
    {
      page_1();
    }

    if (!ok)
    {
      return false;
    }
  }
  return true;
}

void main(int argc, char *argv[])
{
  uint8_t eth_init = ETH_INIT_DEFAULT;
  bool do_again = false;
  bool tape_out = false;
  char *url = NULL;

  _filetype = PRODOS_T_TXT;
  _heapadd((void *)0x0800, 0x1800);
  videomode(VIDEOMODE_80COL);
  atexit(confirm_exit);

  cputsxy(21, 0, "\xDA\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC"
                     "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC"
                     "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xDF");
  cputsxy(21, 1, "\xDA  A2Stream - Oliver Schmidt - 2022  \xDF");
  cputsxy(21, 2, "\xDA\x5F\x5F\x5F\x5F\x5F\x5F\x5F\x5F\x5F\x5F\x5F\x5F"
                     "\x5F\x5F\x5F\x5F\x5F\x5F\x5F\x5F\x5F\x5F\x5F\x5F"
                     "\x5F\x5F\x5F\x5F\x5F\x5F\x5F\x5F\x5F\x5F\x5F\x5F\xDF");

  {
    char cwd[FILENAME_MAX];

    if (!*getcwd(cwd, sizeof(cwd)))
    {
      chdir(getdevicedir(getcurrentdevice(), cwd, sizeof(cwd)));
      printf("\n\nSetting prefix - %s", cwd);
    }
  }

  {
    DIR *dir;
    struct dirent *dirent;

    dir = opendir("/RAM");
    if (!dir)
    {
      printf("\n\n/RAM not present\n");
      exit(EXIT_FAILURE);
    }

    dirent = readdir(dir);
    if (dirent)
    {
      printf("\n\n/RAM not empty\n");
      exit(EXIT_FAILURE);
    }

    closedir(dir);
  }

  {
    int file;

    printf("\n\nSetting slot ");
    file = open("ethernet.slot", O_RDONLY);
    if (file != -1)
    {
      read(file, &eth_init, 1);
      close(file);
      eth_init &= ~'0';
    }
  }

  printf("- %u\n\nInitializing %s ", eth_init, eth_name);
  #ifdef HAVE_ETH
  if (ip65_init(eth_init))
  {
    error_exit();
  }
  #endif

  printf("- Ok\n\nObtaining IP address ");
  #ifdef HAVE_ETH
  if (dhcp_init())
  {
    error_exit();
  }
  #endif
  printf("- Ok\n\n");

  linenoiseHistoryLoad("stream.urls");
  linenoiseSetCompletionCallback(completion);

  // Repeat playing a stream
  while (true)
  {
    if (do_again)
    {
      // Reinit IP65 for DNS lookup,
      // IP configuration still valid
      #ifdef HAVE_ETH
      ip65_init(eth_init);
      #endif
    }

    // Repeat parsing a URL
    while (true)
    {
      // Access command line arg only once
      if (argc > 1 && !url)
      {
        url = argv[1];
        printf("URL: %s", url);
      }
      else
      {
        url = linenoise("URL? ");
        if (!url || !*url)
        {
          putchar('\n');
          exit(EXIT_FAILURE);
        }
      }

      linenoiseHistoryAdd(url);

      printf("\n\nProcessing URL ");
      #ifdef HAVE_ETH
      if (!url_parse(url))
      #endif
      {
        break;
      }

      printf("- %s\n\n", ip65_strerror(ip65_error));
    }

    printf("- Ok\n\nSaving URL ");
    printf("- %s\n\n", linenoiseHistorySave("stream.urls") ? "No" : "Ok");

    if (!do_again)
    {
      int file;

      do_again = true;
      printf("Setting output ");
      file = open("output.tape", O_RDONLY);
      if (file != -1)
      {
        close(file);
        tape_out = true;
      }
      printf("- %s\n\n", tape_out ? "Tape" : "Speaker");

      gen_player(eth_init, tape_out);
      printf("\n\n");
    }

    // Copy IP config from IP65 to W5100
    w5100_config(eth_init);

    {
      bool ok;
      char* buffer = malloc(0x1000);

      #ifdef HAVE_ETH
      ok = w5100_http_open(url_ip, url_port, url_selector, buffer, 0x1000);
      #else
      ok = true;
      #endif

      free(buffer);
      if (!ok)
      {
        putchar('\n');
        continue;
      }
    }

    hires_on();

    printf("- Ok\n\nLoading cover art ");
    if (!load_hires(true) || !load_hires(false))
    {
      printf("- Failed\n\n");
      w5100_disconnect();
      continue;
    }
    printf("- Ok\n\n");

    mix_on();
    dhires();
    text_off();

    {
      char text_1[4][40];
      char text_2[4][40];
      uint8_t y;
      uint8_t cursor_x = wherex();
      uint8_t cursor_y = wherey();

      // Save and clear text lines 20 to 24
      for (y = 0; y < 4; ++y)
      {
        gotoy(20 + y);

        memcpy(text_1[y], *(void **)0x28, 40);
        memset(*(void **)0x28, ' ' | 0x80, 40);
        page_2();
        memcpy(text_2[y], *(void **)0x28, 40);
        memset(*(void **)0x28, ' ' | 0x80, 40);
        page_1();
      }

      cputsxy(31, 21, "\xDA\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC"
                          "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xDF");
      cputsxy(31, 22, "\xDA   Loading...   \xDF");
      cputsxy(31, 23, "\xDA\x5F\x5F\x5F\x5F\x5F\x5F\x5F\x5F"
                          "\x5F\x5F\x5F\x5F\x5F\x5F\x5F\x5F\xDF");

      play();

      // Restore text lines 20 to 24
      for (y = 0; y < 4; ++y)
      {
        gotoy(20 + y);

        memcpy(*(void **)0x28, text_1[y], 40);
        page_2();
        memcpy(*(void **)0x28, text_2[y], 40);
        page_1();
      }
      gotoxy(cursor_x, cursor_y);
    }

    text_on();
    shires();
  }
}
