import OpenAI from 'openai';
import fs from 'fs';
import { getImageDescriptionPrompt, SYSTEM_PROMPT, VOICE_TONE_INSTRUCTIONS } from '../prompts/prompts';

// Initialize the OpenAI client
const openai = new OpenAI({
  apiKey: process.env.OPENAI_API_KEY,
});

export const describeImage = async (imagePath: string, wordCount: number = 75): Promise<string> => {
  try {
    // Read the image as base64
    const imageBuffer = fs.readFileSync(imagePath);
    const base64Image = imageBuffer.toString('base64');

    // Use GPT-4 Vision to analyze the image
    const response = await openai.chat.completions.create({
      model: "gpt-4o-mini",
      messages: [
        {
          role: "system",
          content: SYSTEM_PROMPT
        },
        {
          role: "user",
          content: [
            { type: "text", text: getImageDescriptionPrompt(wordCount) },
            { 
              type: "image_url", 
              image_url: {
                url: `data:image/jpeg;base64,${base64Image}`
              }
            }
          ]
        }
      ],
      max_tokens: 300
    });

    if (!response.choices || response.choices.length === 0) {
      throw new Error('Failed to get a description from the image');
    }

    return response.choices[0].message.content || 'No description available';
  } catch (error) {
    console.error("Error describing image:", error);
    throw error;
  }
};

export const generateAudio = async (text: string): Promise<Buffer> => {
  try {
    // Use OpenAI's TTS endpoint to generate speech
    const response = await openai.audio.speech.create({
      model: "gpt-4o-mini-tts",
      voice: "alloy", // Other options: echo, fable, onyx, nova, shimmer
      input: text,
      instructions: VOICE_TONE_INSTRUCTIONS,
      response_format: "pcm"
    });

    // Get audio data as an ArrayBuffer
    const audioData = await response.arrayBuffer();
    
    // Convert ArrayBuffer to Buffer for API response
    return Buffer.from(audioData);
  } catch (error) {
    console.error("Error generating audio:", error);
    throw error;
  }
};
