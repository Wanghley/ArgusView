import dotenv from 'dotenv';
// Load environment variables as early as possible
dotenv.config();

import express from 'express';
import multer from 'multer';
import path from 'path';
import fs from 'fs';
import { describeImage, generateAudio, transcribeAudio } from './lib/openai';
import { correctAndWrapEsp32AudioAsWav } from './lib/audio';

const app = express();
const port = 3000;

const upload = multer({ dest: 'uploads/' });

app.use(express.json());

app.get('/', (req, res) => {
  res.send('Hello, world!');
});

app.get('/alive', (req, res) => {
  console.log('Received request to /alive');
  console.log(`Request headers: ${JSON.stringify(req.headers)}`);
  console.log(`Request query: ${JSON.stringify(req.query)}`);
  console.log(`Request body: ${JSON.stringify(req.body)}`);
  console.log(`Request method: ${req.method}`);
  res.status(200).send({ status: 'alive' });
});

const IS_DEMO = true; // Flag to control demo mode

const GENERATED_AUDIO_DIR = path.resolve(__dirname, '../uploads/generated');

// Ensure the generated audio directory exists
if (!fs.existsSync(GENERATED_AUDIO_DIR)) {
  fs.mkdirSync(GENERATED_AUDIO_DIR, { recursive: true });
}

/**
 * Sends the audio response to the client.
 * @param res - Express response object
 * @param isDemo - Optional flag to override the global IS_DEMO flag
 */
const sendAudioResponse = (res: express.Response, isDemo: boolean = IS_DEMO, audioFileName: string = 'audio.wav') => {
  const audioPath = isDemo
    ? path.resolve(__dirname, 'response.wav') // Demo audio file
    : path.resolve(GENERATED_AUDIO_DIR, audioFileName); // Path for generated audio

  const audioBuffer = fs.readFileSync(audioPath);
  const stream = fs.createReadStream(audioPath, { highWaterMark: 1024 });

  res.setHeader('Content-Type', 'application/octet-stream');
  res.setHeader('Content-Length', audioBuffer.length.toString());
  res.setHeader('Connection', 'close');

  stream.pipe(res);
};

/**
 * Generates an audio description for the given image and optional audio input.
 * @param imagePath - Path to the image file
 * @param wordCount - Number of words for the description
 * @param additionalContext - Context from the audio file
 * @returns The filename of the generated audio file
 */
const generateAudioDescription = async (
  imagePath: string,
  wordCount: number,
  additionalContext?: string
): Promise<string> => {
  console.log(`Generating audio description for image: ${imagePath}`);

  let audioFileName = 'response.wav'; // Default file name

  if (!IS_DEMO) {
    const description = await describeImage(imagePath, wordCount, additionalContext);
    console.log('Image description generated:', description);
    console.log(`Actual word count: ${description.split(' ').length}`);

    const audioBuffer = await generateAudio(description);
    audioFileName = `audio_${Date.now()}.pcm`;
    const generatedAudioPath = path.resolve(GENERATED_AUDIO_DIR, audioFileName);
    fs.writeFileSync(generatedAudioPath, audioBuffer);
  } else {
    console.log('IS_DEMO is true, skipping audio description generation.');
  }

  return audioFileName;
};

app.post('/describe', upload.single('image'), async (req, res) => {
  console.log(`Received request to describe image: ${req.file?.originalname}`);
  console.log(`File path: ${req.file?.path}`);
  console.log(`File size: ${req.file?.size} bytes`);

  try {
    if (!req.file) {
      return res.status(400).send('No image file uploaded.');
    }

    const imagePath = path.resolve(req.file.path);
    const wordCount = req.query.wordCount ? parseInt(req.query.wordCount as string) : 75;

    const audioFileName = await generateAudioDescription(imagePath, wordCount);
    sendAudioResponse(res, IS_DEMO, audioFileName);
  } catch (error) {
    console.error(error);
    res.status(500).send('An error occurred while processing the image.');
  }
});

const uploadWithContext = multer({ dest: 'uploads/' }).fields([
  { name: 'image', maxCount: 1 },
  { name: 'audio', maxCount: 1 },
]);

app.post('/describe_with_context', uploadWithContext, async (req, res) => {
  console.log(`Received request to describe image with context.`);
  const files = req.files as { [fieldname: string]: Express.Multer.File[] } | undefined;
  const imageFile = files?.['image']?.[0];
  const audioFile = files?.['audio']?.[0];

  if (!imageFile || !audioFile) {
    console.error('Missing image or audio file.');
    return res.status(400).send('Both image and audio files are required.');
  }

  console.log(`Image file: ${imageFile.originalname}, path: ${imageFile.path}, size: ${imageFile.size} bytes`);
  console.log(`Audio file: ${audioFile.originalname}, path: ${audioFile.path}, size: ${audioFile.size} bytes`);

  try {
    const imagePath = path.resolve(imageFile.path);
    const audioPath = path.resolve(audioFile.path);
    const wordCount = req.query.wordCount ? parseInt(req.query.wordCount as string) : 75;

    let additionalContext : string | undefined;

    const correctedWavPath = path.resolve('uploads', 'corrected_context.wav');

    console.log('Correcting and wrapping ESP32 raw audio into WAV...');
    await correctAndWrapEsp32AudioAsWav(audioPath, correctedWavPath);
    console.log('Audio correction and WAV wrapping completed.');

    if (!IS_DEMO) {
      console.log('Transcribing audio for additional context...');
      additionalContext = await transcribeAudio(path.resolve('./context.wav'));
      console.log('Transcription completed:', additionalContext);
    } else {
      additionalContext = await transcribeAudio(correctedWavPath);
      console.log('Transcription completed:', additionalContext);
      console.log('IS_DEMO is true, skipping audio transcription.');
    }

    const audioFileName = await generateAudioDescription(imagePath, wordCount, additionalContext);
    sendAudioResponse(res, IS_DEMO, audioFileName);
  } catch (error) {
    console.error(error);
    res.status(500).send('An error occurred while processing the image and audio.');
  }
});

app.post('/log', (req, res) => {
  const { level, message } = req.body;

  if (!level || !message) {
    return res.status(400).send({ error: 'Both level and message are required.' });
  }

  console.log(`\x1b[34m[${level.toUpperCase()}] ${message}\x1b[0m`);
  res.status(200).send({ status: 'Log received' });
});

app.listen(port, () => {
  console.log(`Server is running on http://localhost:${port}`);
});
