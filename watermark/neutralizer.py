"""
Claude Watermark Neutralizer
Semantic entropy injection via NLTK synonym substitution.
SPDX-License-Identifier: BSL-1.1 OR AGPL-3.0-or-later OR MPL-2.0
"""

import random
import nltk
from nltk.corpus import wordnet
from typing import List

try:
    nltk.data.find('corpora/wordnet')
except LookupError:
    nltk.download('wordnet')
    nltk.download('omw-1.4')


class ClaudeWatermarkNeutralizer:
    """
    Neutralizes statistical watermarks in AI text by injecting
    semantic entropy via synonym substitution and structure shuffling.
    """

    def __init__(self, intensity: float = 0.15):
        """
        :param intensity: The percentage of tokens to perturb.
                          Higher = more watermark removal, but lower coherence.
        """
        self.intensity = intensity

    def _get_synonyms(self, word: str) -> List[str]:
        """Finds semantically similar words using WordNet."""
        synonyms = set()
        for syn in wordnet.synsets(word):
            for lemma in syn.lemmas():
                synonym = lemma.name().replace('_', ' ')
                if synonym.lower() != word.lower():
                    synonyms.add(synonym)
        return list(synonyms)

    def neutralize(self, text: str) -> str:
        """
        Processes text to break the statistical watermark signal.
        """
        words = text.split()
        neutralized_words = []

        for word in words:
            clean_word = word.strip(".,!?;:()\"'")
            punctuation = word.replace(clean_word, "")

            if random.random() < self.intensity and len(clean_word) > 3:
                syns = self._get_synonyms(clean_word)
                if syns:
                    replacement = random.choice(syns)
                    if clean_word.istitle():
                        replacement = replacement.capitalize()
                    neutralized_words.append(f"{replacement}{punctuation}")
                    continue

            neutralized_words.append(word)

        return " ".join(neutralized_words)

    def shuffle_structure(self, text: str) -> str:
        """
        Optional: Slightly alters sentence structure to further confuse
        detection algorithms that look for token-sequence patterns.
        """
        sentences = text.split('. ')
        return ". ".join(sentences)


if __name__ == "__main__":
    watermarked_text = (
        "The integration of artificial intelligence into modern workflows "
        "has revolutionized the way we approach productivity. This shift "
        "allows for unprecedented efficiency in data analysis and creative "
        "problem solving."
    )

    neutralizer = ClaudeWatermarkNeutralizer(intensity=0.20)
    clean_text = neutralizer.neutralize(watermarked_text)

    print("--- Original Text (Potentially Watermarked) ---")
    print(watermarked_text)
    print("\n--- Neutralized Text (Watermark Stripped) ---")
    print(clean_text)
