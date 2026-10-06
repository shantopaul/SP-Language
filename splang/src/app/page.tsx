import { HeroSection } from "@/components/sections/HeroSection";
import { CodeShowcase } from "@/components/sections/CodeShowcase";
import { KeyFeatures } from "@/components/sections/KeyFeatures";
import { BenchmarkVisualizer } from "@/components/sections/BenchmarkVisualizer";
import { ToolingShowcase } from "@/components/sections/ToolingShowcase";
import { EcosystemGrid } from "@/components/sections/EcosystemGrid";
import { ReleaseCallout } from "@/components/sections/ReleaseCallout";

export default function HomePage() {
  return (
    <div className="flex flex-col">
      <HeroSection />
      <CodeShowcase />
      <KeyFeatures />
      <BenchmarkVisualizer />
      <ToolingShowcase />
      <EcosystemGrid />
      <ReleaseCallout />
    </div>
  );
}
