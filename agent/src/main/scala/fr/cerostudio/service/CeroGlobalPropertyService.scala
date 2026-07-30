package fr.cerostudio.service

import org.spongepowered.asm.service.{IGlobalPropertyService, IPropertyKey}

import scala.collection.concurrent.TrieMap

/**
 * Implémentation Scala du service de propriétés globales Mixin.
 * Utilise un TrieMap (concurrent, thread-safe) au lieu du ConcurrentHashMap Java.
 */
class CeroGlobalPropertyService extends IGlobalPropertyService {

  private val properties = TrieMap[String, Object]()

  /** Représentation type-safe d'une clé de propriété */
  private final class Key(val name: String) extends IPropertyKey {
    override def equals(obj: Any): Boolean = obj match {
      case other: Key => other.name == this.name
      case _           => false
    }

    override def hashCode(): Int = name.hashCode

    override def toString: String = name
  }

  override def resolveKey(name: String): IPropertyKey = new Key(name)

  override def getProperty[T](key: IPropertyKey): T = {
    key.asInstanceOf[Key].name -> properties.get(key.asInstanceOf[Key].name) match {
      case (_, Some(value)) => value.asInstanceOf[T]
      case _ => null.asInstanceOf[T]
    }
  }

  override def setProperty(key: IPropertyKey, value: Any): Unit = {
    properties.put(key.asInstanceOf[Key].name, value.asInstanceOf[Object])
  }

  override def getProperty[T](key: IPropertyKey, defaultValue: T): T = {
    properties.getOrElse(key.asInstanceOf[Key].name, defaultValue.asInstanceOf[Object]).asInstanceOf[T]
  }

  override def getPropertyString(key: IPropertyKey, defaultValue: String): String = {
    properties.get(key.asInstanceOf[Key].name) match {
      case Some(value) => value.toString
      case None        => defaultValue
    }
  }
}
