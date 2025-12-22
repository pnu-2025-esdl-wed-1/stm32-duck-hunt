package com.example.esdl_term_project

data class GameStatus(
    val score: Int,
    val bullets: Int
)

object MessageParser {
    /**
     * Parses a STATUS message from the STM32.
     * Expected format: "STATUS,score=20,bullets=5"
     * Returns null if format is invalid or not a STATUS message.
     */
    fun parseStatus(message: String): GameStatus? {
        // New Protocol: TYPE=200;SCORE=100;BULLETS=3
        if (message.startsWith("TYPE=200")) {
            try {
                val parts = message.split(";")
                var score = 0
                var bullets = 3
                
                for (part in parts) {
                    when {
                        part.startsWith("SCORE=") -> {
                            score = part.substringAfter("SCORE=").toIntOrNull() ?: 0
                        }
                        part.startsWith("BULLETS=") -> {
                            bullets = part.substringAfter("BULLETS=").toIntOrNull() ?: 3
                        }
                    }
                }
                return GameStatus(score, bullets)
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }

        // Legacy Protocol: STATUS,score=20,bullets=5
        if (message.startsWith("STATUS")) {
            try {
                val parts = message.split(",")
                var score = 0
                var bullets = 3 // Default to max ammo

                for (part in parts) {
                    if (part.startsWith("score=")) {
                        score = part.substringAfter("score=").toInt()
                    } else if (part.startsWith("bullets=")) {
                        bullets = part.substringAfter("bullets=").toInt()
                    }
                }
                return GameStatus(score, bullets)
            } catch (e: Exception) {
                e.printStackTrace()
                return null
            }
        }
        return null
    }
}
