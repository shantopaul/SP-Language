import { SEARCH_INDEX, SearchItem } from "@/data/search-items";

export function searchDocs(query: string): SearchItem[] {
  const cleanQuery = query.trim().toLowerCase();
  if (!cleanQuery) return [];

  const terms = cleanQuery.split(/\s+/);

  return SEARCH_INDEX.filter((item) => {
    const titleMatch = item.title.toLowerCase();
    const excerptMatch = item.excerpt.toLowerCase();
    const categoryMatch = item.category.toLowerCase();
    const keywords = item.keywords.map((k) => k.toLowerCase());

    return terms.every(
      (term) =>
        titleMatch.includes(term) ||
        excerptMatch.includes(term) ||
        categoryMatch.includes(term) ||
        keywords.some((k) => k.includes(term))
    );
  }).slice(0, 8); // Top 8 results
}
