import path from 'path';
import { correctAndWrapEsp32AudioAsWav } from '../lib/audio';

const inputFilePath = process.argv[2];
if (!inputFilePath) {
  console.error('Error: Please provide the input file path as an argument.');
  process.exit(1);
}

const outputFilePath = path.resolve(
  path.dirname(inputFilePath),
  'corrected_' + path.basename(inputFilePath, path.extname(inputFilePath)) + '.wav'
);

(async () => {
  try {
    console.log(`Correcting and wrapping audio: ${inputFilePath}`);
    await correctAndWrapEsp32AudioAsWav(inputFilePath, outputFilePath);
    console.log(`Corrected WAV file saved at: ${outputFilePath}`);
  } catch (error) {
    console.error('Error during audio correction:', error);
    process.exit(1);
  }
})();
