import dotenv from 'dotenv';
// Load environment variables as early as possible
dotenv.config();

import express from 'express';
import multer from 'multer';
import path from 'path';
import { describeImage, generateAudio } from './lib/openai';

const app = express();
const port = 3000;

const upload = multer({ dest: 'uploads/' });

app.use(express.json());

app.get('/', (req, res) => {
  res.send('Hello, world!');
});

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
    
    console.log(`Starting to describe the image with around ${wordCount} words...`);
    // const description = await describeImage(imagePath, wordCount);
    // console.log('Image description generated:', description);
    // console.log(`Actual word count: ${description.split(' ').length}`);
    
    // const audioBuffer = await generateAudio(description);
    const audioBuffer = require('fs').readFileSync(path.resolve(__dirname, 'response.wav'));

    // Actually these are raw PCM data...
    res.set('Content-Type', 'audio/wav');
    res.send(audioBuffer);

    // res.status(200).json({ description });
  } catch (error) {
    console.error(error);
    res.status(500).send('An error occurred while processing the image.');
  }
});

app.listen(port, () => {
  console.log(`Server is running on http://localhost:${port}`);
});
