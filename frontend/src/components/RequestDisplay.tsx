'use client';

import { useState, useEffect } from 'react';
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Skeleton } from "@/components/ui/skeleton";
import { fetchLatestRequest, LatestRequest } from '@/lib/api';

export default function RequestDisplay() {
  const [data, setData] = useState<LatestRequest>({});
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const fetchData = async () => {
    try {
      setLoading(true);
      setError(null);
      const latestData = await fetchLatestRequest();
      setData(latestData);
    } catch (err) {
      setError('Failed to load the latest request data');
      console.error(err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    // Initial fetch only, no auto-polling
    fetchData();
  }, []);

  return (
    <div className="space-y-6 w-full max-w-6xl mx-auto">
      <div className="flex justify-between items-center">
        <h2 className="text-2xl font-bold">Latest Request</h2>
        <Button onClick={fetchData} disabled={loading}>Refresh</Button>
      </div>
      
      {error && (
        <div className="p-4 bg-red-100 text-red-800 rounded-md">
          {error}
        </div>
      )}

      {loading ? (
        <LoadingState />
      ) : (
        <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
          <ImageSection imageUrl={data.imageUrl} />
          <AudioSection 
            audioInput={data.audioInput} 
            audioTranscription={data.audioTranscription} 
            audioOutput={data.audioOutput} 
          />
        </div>
      )}
    </div>
  );
}

function LoadingState() {
  return (
    <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
      <Card>
        <CardHeader>
          <Skeleton className="h-6 w-1/3" />
        </CardHeader>
        <CardContent>
          <Skeleton className="h-64 w-full rounded-md" />
        </CardContent>
      </Card>
      <Card>
        <CardHeader>
          <Skeleton className="h-6 w-1/3" />
        </CardHeader>
        <CardContent className="space-y-4">
          <Skeleton className="h-10 w-full" />
          <Skeleton className="h-20 w-full" />
          <Skeleton className="h-10 w-full" />
        </CardContent>
      </Card>
    </div>
  );
}

function ImageSection({ imageUrl }: { imageUrl?: string }) {
  return (
    <Card>
      <CardHeader>
        <CardTitle>Image</CardTitle>
      </CardHeader>
      <CardContent className="flex justify-center">
        {imageUrl ? (
          <div className="overflow-hidden rounded-md">
            <img 
              src={`${imageUrl}`} 
              alt="Captured image" 
              className="w-full max-h-96 object-contain"
            />
          </div>
        ) : (
          <div className="h-64 w-full flex items-center justify-center bg-gray-100 text-gray-500 rounded-md">
            No image available
          </div>
        )}
      </CardContent>
    </Card>
  );
}

function AudioSection({ 
  audioInput, 
  audioTranscription, 
  audioOutput 
}: { 
  audioInput?: string; 
  audioTranscription?: string; 
  audioOutput?: string; 
}) {
  return (
    <Card>
      <CardHeader>
        <CardTitle>Audio Processing</CardTitle>
      </CardHeader>
      <CardContent className="space-y-4">
        <div className="space-y-2">
          <h3 className="font-medium">Audio Input</h3>
          {audioInput ? (
            <audio controls className="w-full">
              <source src={audioInput} type="audio/wav" />
              Your browser does not support the audio element.
            </audio>
          ) : (
            <div className="text-gray-500">No audio input available</div>
          )}
        </div>

        <div className="space-y-2">
          <h3 className="font-medium">Transcription</h3>
          {audioTranscription ? (
            <div className="p-3 bg-gray-50 dark:bg-gray-800 rounded-md">
              {audioTranscription}
            </div>
          ) : (
            <div className="text-gray-500">No transcription available</div>
          )}
        </div>

        <div className="space-y-2">
          <h3 className="font-medium">Audio Output</h3>
          {audioOutput ? (
            <audio controls className="w-full">
              <source src={audioOutput} type="audio/wav" />
              Your browser does not support the audio element.
            </audio>
          ) : (
            <div className="text-gray-500">No audio output available</div>
          )}
        </div>
      </CardContent>
    </Card>
  );
}