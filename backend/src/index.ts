import dotenv from 'dotenv';
// Load environment variables as early as possible
dotenv.config();

import express from 'express';
import multer from 'multer';
import path from 'path';
import fs from 'fs';
import { describeImage, generateAudio, transcribeAudio } from './lib/openai';
import { correctAndWrapEsp32AudioAsWav, convertPcmToWav } from './lib/audio';
import cors from 'cors';

const app = express();
const port = 3000;

// Global variable to store the latest request information
let latestRequest: {
  imageUrl?: string;
  audioInput?: string;
  audioTranscription?: string;
  audioOutput?: string;
  timestamp?: string;
} = {};

const upload = multer({ dest: 'uploads/' });

app.use(express.json());
app.use(cors()); // Enable CORS for all origins
app.use('/uploads', express.static(path.join(__dirname, '../uploads'))); // Serve static files from uploads directory

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

const IS_DEMO = false; // Flag to control demo mode

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
const sendAudioResponse = (res: express.Response, isDemo: boolean = IS_DEMO, audioPath: string) => {
  const resolvedAudioPath = path.resolve(__dirname, audioPath)
  const audioBuffer = fs.readFileSync(resolvedAudioPath);
  const stream = fs.createReadStream(resolvedAudioPath, { highWaterMark: 1024 });  

  res.setHeader('Content-Type', 'application/octet-stream');
  res.setHeader('Content-Length', audioBuffer.length.toString());
  res.setHeader('Connection', 'close');

  stream.pipe(res);
};

/**
 * Updates the latestRequest global variable with new request information
 */
const updateLatestRequest = async (
  imageUrl?: string,
  audioInput?: string,
  audioTranscription?: string, 
  audioOutput?: string
) => {
  let convertedAudioOutput: string | undefined = undefined;
  
  // Convert PCM audio to WAV for browser playback if available
  if (audioOutput) {
    try {
      convertedAudioOutput = await convertPcmToWav(audioOutput, GENERATED_AUDIO_DIR);
      console.log('Converted audio for browser playback:', convertedAudioOutput);
    } catch (error) {
      console.error('Failed to convert audio for browser playback:', error);
      // Fall back to original audio if conversion fails
      convertedAudioOutput = audioOutput;
    }
  }
  
  latestRequest = {
    imageUrl: imageUrl ? `/uploads/${path.basename(imageUrl)}` : undefined,
    audioInput: audioInput ? `/uploads/${path.basename(audioInput)}` : undefined,
    audioTranscription,
    audioOutput: convertedAudioOutput,
    timestamp: new Date().toISOString()
  };
  
  console.log('Latest request updated:', latestRequest);
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

    return generatedAudioPath;
  } else {
    console.log('IS_DEMO is true, skipping audio description generation.');
  }

  return path.resolve(GENERATED_AUDIO_DIR, audioFileName);
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

    const audioPath = await generateAudioDescription(imagePath, wordCount);
    
    // Update latest request with image path
    updateLatestRequest(imagePath, undefined, undefined, audioPath);
    
    sendAudioResponse(res, IS_DEMO, audioPath);
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
      additionalContext = await transcribeAudio(correctedWavPath);
      console.log('Transcription completed:', additionalContext);
    } else {
      additionalContext = await transcribeAudio(correctedWavPath);
      console.log('Transcription completed:', additionalContext);
      console.log('IS_DEMO is true, skipping audio transcription.');
    } 

    const outputAudioPath = await generateAudioDescription(imagePath, wordCount, additionalContext);
    
    // Update latest request with all information
    updateLatestRequest(
      imagePath,
      correctedWavPath,
      additionalContext,
      outputAudioPath
    );
    
    sendAudioResponse(res, IS_DEMO, outputAudioPath);
  } catch (error) {
    console.error(error);
    res.status(500).send('An error occurred while processing the image and audio.');
  }
});

// Endpoint to retrieve the latest request information
app.get('/latest-request', (req, res) => {
  console.log('Received request to /latest-request');
  res.status(200).json(latestRequest);
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
