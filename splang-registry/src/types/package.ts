export interface PackageVersion {
  version: string;
  publishedAt: string;
  checksum: string;
  size: string;
  spmMinVersion: string;
  changelog?: string;
}

export interface PackageDependency {
  name: string;
  req: string;
  optional?: boolean;
}

export interface Package {
  name: string;
  version: string;
  description: string;
  author: string;
  license: string;
  category: string;
  keywords: string[];
  totalDownloads: number;
  publishedAt: string;
  readme: string;
  dependencies: PackageDependency[];
  versions: PackageVersion[];
  repositoryUrl: string;
  official?: boolean;
}

export interface Category {
  id: string;
  name: string;
  description: string;
  iconName: string;
}
