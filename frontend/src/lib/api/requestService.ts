/**
 * API service to fetch latest request data from the backend
 */

export interface RequestData {
  imageUrl?: string;
  audioInput?: string;
  audioTranscription?: string;
  audioOutput?: string;
  timestamp?: string;
}

/**
 * Fetches the latest request data from the backend
 */
export async function fetchLatestRequest(): Promise<RequestData> {
  try {
    const response = await fetch('http://vcm-46428.vm.duke.edu:3000/latest-request');
    
    if (!response.ok) {
      throw new Error(`Error fetching latest request: ${response.status}`);
    }
    
    return await response.json();
  } catch (error) {
    console.error('Failed to fetch latest request:', error);
    return {}; // Return empty object on error
  }
}