plugins {
    id("java")
    id("com.gradleup.shadow") version "8.3.5"
}

group = "fr.cerostudio"
version = "1.0-SNAPSHOT"

java {
    sourceCompatibility = JavaVersion.VERSION_1_8
    targetCompatibility = JavaVersion.VERSION_1_8
}

repositories {
    mavenCentral()
    maven("https://repo.spongepowered.org/maven/")
}

dependencies {
    implementation("org.spongepowered:mixin:0.8.5")
    implementation("net.fabricmc:tiny-remapper:0.10.4")
    
    implementation("org.ow2.asm:asm:9.7")
    implementation("org.ow2.asm:asm-commons:9.7")
    implementation("org.ow2.asm:asm-tree:9.7")
    implementation("org.ow2.asm:asm-util:9.7")

    implementation("com.google.guava:guava:32.1.3-jre")

    testImplementation(platform("org.junit:junit-bom:5.10.0"))
    testImplementation("org.junit.jupiter:junit-jupiter")
    testRuntimeOnly("org.junit.platform:junit-platform-launcher")
}

tasks.test {
    useJUnitPlatform()
}

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