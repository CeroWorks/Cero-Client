plugins {
    id("java")
    id("scala")
    id("com.gradleup.shadow") version "8.3.5"
}

group = "fr.cerostudio"
version = "1.0-SNAPSHOT"

java {
    sourceCompatibility = JavaVersion.VERSION_1_8
    targetCompatibility = JavaVersion.VERSION_1_8
}

// ── Scala configuration ──────────────────────────────────────────────
scala {
    zincVersion = "1.10.7"
}

repositories {
    mavenCentral()
    maven("https://repo.spongepowered.org/maven/")
}

dependencies {
    // Scala 2.13 — compatible avec Java 8 target
    implementation("org.scala-lang:scala-library:2.13.16")

    // SpongePowered Mixin (obligatoirement Java pour les classes mixin)
    implementation("org.spongepowered:mixin:0.8.5")

    // TinyRemapper
    implementation("net.fabricmc:tiny-remapper:0.10.4")

    // ASM
    implementation("org.ow2.asm:asm:9.7")
    implementation("org.ow2.asm:asm-commons:9.7")
    implementation("org.ow2.asm:asm-tree:9.7")
    implementation("org.ow2.asm:asm-util:9.7")

    // Guava
    implementation("com.google.guava:guava:32.1.3-jre")

    // Tests
    testImplementation(platform("org.junit:junit-bom:5.10.0"))
    testImplementation("org.junit.jupiter:junit-jupiter")
    testImplementation("org.scalatest:scalatest_2.13:3.2.19")
    testRuntimeOnly("org.junit.platform:junit-platform-launcher")
}

// ── Compilation mixte Java + Scala ──────────────────────────────────
// Les sources Scala peuvent appeler du code Java compilé au préalable.
tasks.withType<ScalaCompile> {
    scalaCompileOptions.additionalParameters = listOf("-target:jvm-1.8")
}

tasks.test {
    useJUnitPlatform()
}

// ── Shadow JAR ───────────────────────────────────────────────────────
tasks.shadowJar {
    archiveBaseName.set("CeroClient-MC")
    archiveVersion.set("")
    archiveClassifier.set("")

    manifest {
        attributes(
            "Main-Class" to "fr.cerostudio.Main",
            "MixinConfigs" to "mixins.cero.json"
        )
    }

    relocate("com.google.common", "fr.cerostudio.libs.guava")
    relocate("org.objectweb.asm", "fr.cerostudio.libs.asm")
    // Scala library est incluse via relocation pour éviter les conflits
    relocate("scala.", "fr.cerostudio.libs.scala.")

    exclude("META-INF/versions/**")
    exclude("module-info.class")

    mergeServiceFiles()
}

tasks.jar {
    enabled = false
}

tasks.build {
    dependsOn(tasks.shadowJar)
}
