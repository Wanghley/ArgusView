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
  try {
    if (!req.file) {
      return res.status(400).send('No image file uploaded.');
    }

    const imagePath = path.resolve(req.file.path);
    const wordCount = req.query.wordCount ? parseInt(req.query.wordCount as string) : 75;
    
    console.log(`Starting to describe the image with around ${wordCount} words...`);
    const description = await describeImage(imagePath, wordCount);
    console.log('Image description generated:', description);
    console.log(`Actual word count: ${description.split(' ').length}`);
    const audioBuffer = await generateAudio(description);

    res.set('Content-Type', 'audio/mpeg');
    res.send(audioBuffer);
  } catch (error) {
    console.error(error);
    res.status(500).send('An error occurred while processing the image.');
  }
});

app.listen(port, () => {
  console.log(`Server is running on http://localhost:${port}`);
});
