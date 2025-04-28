import RequestDisplay from "@/components/RequestDisplay";

export default function Home() {
  return (
    <div className="min-h-screen p-8 pb-20 sm:p-20 font-[family-name:var(--font-geist-sans)]">
      <header className="mb-10">
        <h1 className="text-3xl font-bold">ArgusView Dashboard</h1>
        <p className="text-gray-500 mt-2">
          Monitor the latest requests sent to the backend
        </p>
      </header>
      
      <main>
        <RequestDisplay />
      </main>
    </div>
  );
}
