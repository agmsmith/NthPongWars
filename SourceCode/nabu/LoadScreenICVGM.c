/* We always read whole lines, with 16 numbers per line.  A typical one is:
NAME    DB      $20,$80,$80,$80,$8A,$20,$8B,$8B,$8B,$95,$20,$96,$96,$96,$A0,$20
*/
#define ICVGM_CHARS_PER_LINE 81 /* 79 data, then CRLF. */
#define ICVGM_LINES_PER_BUFFER (TEMPBUFFER_LEN / ICVGM_CHARS_PER_LINE)
#define ICVGM_DATA_BYTES_PER_LINE 16

/*******************************************************************************
 * Internal utility function to read a number of bytes from a file handle as
 * ASCII text, and decode them into a buffer.  Checks that some line starts
 * with the keyword (usually NAME, PATTERN, MCOLOR, SPATT, SCOLOR in that
 * order).  Returns TRUE if successful, FALSE if it runs out of data or the
 * keyword wasn't found.
 */
static bool CopyICVGMToRAM(uint8_t fileId, const char *KeyWord,
uint16_t AmountToRead, uint8_t *DecodedBuffer)
{
  uint16_t amountRemaining;
  char *pBuffer;
  uint8_t *pDecoded;
  uint16_t count;
  char letter;
  uint8_t number;
  bool keyword_found;
  uint8_t keyword_length;
  bool dollar_found;
  bool at_start_of_line;

  memset(TempBuffer, 0, TEMPBUFFER_LEN); /* Make sure no garbage after text. */
  keyword_length = strlen(KeyWord);
  pDecoded = DecodedBuffer;

  amountRemaining = AmountToRead;
  keyword_found = false;
  dollar_found = false;
  number = 0;
  at_start_of_line = true;
  while (amountRemaining != 0) {
    if (amountRemaining > ICVGM_LINES_PER_BUFFER * ICVGM_DATA_BYTES_PER_LINE)
      count = ICVGM_LINES_PER_BUFFER * ICVGM_CHARS_PER_LINE;
    else /* Near the end, just read one line at a time. */
      count = ICVGM_CHARS_PER_LINE;

    count = rn_fileHandleReadSeq(fileId, TempBuffer, 0, count);
    if (count == 0)
      break; /* End of file or error. */

    if (at_start_of_line)
    {
      if (strncmp(KeyWord, TempBuffer, keyword_length) == 0)
        keyword_found = true;
      at_start_of_line = false;
    }

    pBuffer = TempBuffer;
    for (; count != 0; count--)
    {
      letter = *pBuffer++;
      if (letter == '$')
      {
        dollar_found = true;
        number = 0;
      }
      else if (letter >= '0' && letter <= '9')
      {
        if (dollar_found)
          number = number << 4 | (letter - '0');
      }
      else if (letter >= 'A' && letter <= 'F')
      {
        if (dollar_found)
          number = number << 4 | (letter - ('A' - 10));
      }
      else /* Some other characters, including CRLF, maybe number end? */
      {
        if (dollar_found)
        {
          *pDecoded++ = number;
          amountRemaining--;
          dollar_found = false;
        }
      }

      if (letter == 13 || letter == 10) /* CRLF end of line marker. */
        at_start_of_line = true;
    }
  }
  return (amountRemaining == 0) && keyword_found; /* True if successful. */
}

static void CopyVRAMToRAM(uint16_t AmountToCopy, uint8_t *RAMBuffer)
{
  while (AmountToCopy-- != 0)
    IO_VDPDATA = *RAMBuffer++;
}


/*******************************************************************************
 * Read a full screen picture, with sprites etc, into video ram.
 * Uses graphics mode 2, the file is *.DAT created by ICVGM (a Windows tool)
 * which is a hex ASCII listing of the video memory layout.  It contains a name
 * table for the screen tiles, a pattern table for the font, a colour table for
 * the font, the sprite pattern table, and a sprite colour table (not quite
 * sprite attributes, we ignore it).
 *
 * Uses TempBuffer[TEMPBUFFER_LEN] defined by the main program.
 * Returns true if successful, false if it couldn't open the file or there
 * isn't enough data in the file or it doesn't have keywords.
 */

bool LoadScreenICVGM(const char *FileName)
{
  bool returnCode;
  uint8_t fileId;
  uint8_t nameLen;
  uint8_t *decodedDataBuffer;

  returnCode = false;
  decodedDataBuffer = NULL;
  nameLen = strlen(FileName);
  fileId = rn_fileOpen(nameLen, (char *) FileName, OPEN_FILE_FLAG_READONLY,
    0xff /* Use a new file handle */);
  if (fileId == 0xff)
    return false; /* Not found? Wants uppercase, backslashes for directories! */

  /* Need to load a font or colours for a font into RAM, then triplicate them
     into video memory.  Sprite patterns too, though not triplicated. */

  decodedDataBuffer = malloc(0x800);
  if (decodedDataBuffer == NULL)
    goto ErrorExit;

  playNoteDelay(1, 35, 100);

  /* Load name table. */

  if (!CopyICVGMToRAM(fileId, "NAME", 768, decodedDataBuffer))
    goto ErrorExit;
  vdp_setWriteAddress(_vdpPatternNameTableAddr);
  CopyVRAMToRAM(768, decodedDataBuffer);

  /* Load tile patterns. */

  if (!CopyICVGMToRAM(fileId, "PATTERN", 0x800, decodedDataBuffer))
    goto ErrorExit;
  vdp_setWriteAddress(_vdpPatternGeneratorTableAddr);
  CopyVRAMToRAM(0x800, decodedDataBuffer);
  CopyVRAMToRAM(0x800, decodedDataBuffer);
  CopyVRAMToRAM(0x800, decodedDataBuffer);

  /* Load tile colours. */

  if (!CopyICVGMToRAM(fileId, "MCOLOR", 0x800, decodedDataBuffer))
    goto ErrorExit;
  vdp_setWriteAddress(_vdpColorTableAddr);
  CopyVRAMToRAM(0x800, decodedDataBuffer);
  CopyVRAMToRAM(0x800, decodedDataBuffer);
  CopyVRAMToRAM(0x800, decodedDataBuffer);

  /* Load sprite patterns. */

  if (!CopyICVGMToRAM(fileId, "SPATT", 0x800, decodedDataBuffer))
    goto ErrorExit;
  vdp_setWriteAddress(_vdpSpriteGeneratorTableAddr);
  CopyVRAMToRAM(0x800, decodedDataBuffer);

  returnCode = true;

ErrorExit:
  free(decodedDataBuffer);
  rn_fileHandleClose(fileId);
  return returnCode;
}

