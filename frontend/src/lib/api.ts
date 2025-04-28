export interface LatestRequest {
  imageUrl?: string;
  audioInput?: string;
  audioTranscription?: string;
  audioOutput?: string;
  timestamp?: string;
}

/**
 * Fetches the latest request data from the backend
 */
export async function fetchLatestRequest(): Promise<LatestRequest> {
  try {
    const response = await fetch('http://vcm-46428.vm.duke.edu:3000/latest-request');
    
    if (!response.ok) {
      throw new Error(`Error fetching latest request: ${response.status}`);
    }
    
    const data = await response.json();
    
    // Add full DNS for all URLs and .jpg extension for images
    if (data.imageUrl) {
      data.imageUrl = `http://vcm-46428.vm.duke.edu:3000${data.imageUrl}`;
    }
    if (data.audioInput) {
      data.audioInput = `http://vcm-46428.vm.duke.edu:3000${data.audioInput}`;
    }
    if (data.audioOutput) {
      data.audioOutput = `http://vcm-46428.vm.duke.edu:3000${data.audioOutput}`;
    }
    
    return data;
  } catch (error) {
    console.error('Failed to fetch latest request:', error);
    return {}; // Return empty object on error
  }
}