import fs from 'fs';
import path from 'path';
import ffmpegPath from 'ffmpeg-static';
import ffmpeg from 'fluent-ffmpeg';

// Tell fluent-ffmpeg to use the bundled ffmpeg binary
ffmpeg.setFfmpegPath(ffmpegPath!);

/**
 * Corrects raw ESP32 ADC audio and saves it as a proper WAV file, with options to adjust gain and speed.
 * @param inputPath Path to raw input file
 * @param outputPath Path to output WAV file
 * @param sampleRate Audio sample rate (default 16000)
 * @param gain Gain multiplier (default 16)
 * @param speed Speed factor (1 = normal speed, <1 = slower, >1 = faster)
 */
export async function correctAndWrapEsp32AudioAsWav(
  inputPath: string,
  outputPath: string,
  sampleRate = 16000,
  gain = 14,
  speed = 0.6
): Promise<void> {
  const tempWavPath = path.resolve(path.dirname(outputPath), 'temp_corrected.wav');

  // Step 1: Read and correct the raw ESP32 audio
  const rawBuffer = fs.readFileSync(inputPath);
  const correctedBuffer = Buffer.alloc(rawBuffer.length);

  for (let i = 0; i < rawBuffer.length; i += 2) {
    if (i + 1 >= rawBuffer.length) break;

    const sample = rawBuffer.readUInt16LE(i);
    let centeredSample = sample - 2048;
    centeredSample *= gain;

    // Clamp
    if (centeredSample > 32767) centeredSample = 32767;
    if (centeredSample < -32768) centeredSample = -32768;

    correctedBuffer.writeInt16LE(centeredSample, i);
  }

  // Create WAV header
  const header = Buffer.alloc(44);
  const dataSize = correctedBuffer.length;

  header.write('RIFF', 0);
  header.writeUInt32LE(36 + dataSize, 4);
  header.write('WAVE', 8);
  header.write('fmt ', 12);
  header.writeUInt32LE(16, 16);
  header.writeUInt16LE(1, 20);
  header.writeUInt16LE(1, 22);
  header.writeUInt32LE(sampleRate, 24);
  header.writeUInt32LE(sampleRate * 2, 28);
  header.writeUInt16LE(2, 32);
  header.writeUInt16LE(16, 34);
  header.write('data', 36);
  header.writeUInt32LE(dataSize, 40);

  const finalBuffer = Buffer.concat([header, correctedBuffer]);

  fs.writeFileSync(tempWavPath, finalBuffer);

  // Step 2: Use bundled ffmpeg to adjust speed
  return new Promise<void>((resolve, reject) => {
    ffmpeg(tempWavPath)
        .inputOptions([
            '-f', 's16le',
            '-ar', '12000',
            '-ac', '1'
        ])
      .audioCodec('pcm_s16le')
      .audioFilters([
        'aresample=16000',
        'highpass=f=100',
        // 'asetrate=14000',
        'afftdn',
        'volume=2.5',
        'atempo=0.8'
        ])
      .on('error', (err: any) => {
        console.error('FFmpeg error:', err);
        reject(err);
      })
      .on('end', () => {
        console.log('Corrected and adjusted audio saved:', outputPath);
        fs.unlinkSync(tempWavPath); // Clean up temp file
        resolve();
      })
      .save(outputPath);
  });
}

/**
 * Converts PCM audio to WAV format for browser playback
 * @param pcmPath Path to the PCM audio file
 * @param outputDir Directory to save the converted WAV file (optional)
 * @returns Path to the converted WAV file
 */
export async function convertPcmToWav(
  pcmPath: string,
  outputDir?: string
): Promise<string> {
  const outputFileName = `converted_response_for_preview_${Date.now()}.wav`;
  const targetDir = outputDir || path.dirname(pcmPath);
  const outputPath = path.resolve(targetDir, outputFileName);
  
  return new Promise<string>((resolve, reject) => {
    ffmpeg(pcmPath)
      .inputOptions([
        '-f', 's16le',  // PCM format
        '-ar', '24000', // Sample rate for OpenAI TTS
        '-ac', '1'      // Mono channel
      ])
      .audioCodec('pcm_s16le')
      .toFormat('wav')
      .on('error', (err: any) => {
        console.error('Error converting PCM to WAV:', err);
        reject(err);
      })
      .on('end', () => {
        console.log('Successfully converted PCM to WAV! ', `/uploads/generated/${outputFileName}`);
        resolve(`/uploads/generated/${outputFileName}`);
      })
      .save(outputPath);
  });
}
