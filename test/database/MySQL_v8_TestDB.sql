CREATE DATABASE  IF NOT EXISTS `TestDB` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci */ /*!80016 DEFAULT ENCRYPTION='N' */;
USE `TestDB`;
-- MySQL dump 10.13  Distrib 8.0.29, for Linux (x86_64)
--
-- Host: 127.0.0.1    Database: TestDB
-- ------------------------------------------------------
-- Server version	8.0.29

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `bizCar`
--

DROP TABLE IF EXISTS `bizCar`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `bizCar` (
  `id` int NOT NULL AUTO_INCREMENT,
  `name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `price` int DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=4 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `bizCar`
--

LOCK TABLES `bizCar` WRITE;
/*!40000 ALTER TABLE `bizCar` DISABLE KEYS */;
/*!40000 ALTER TABLE `bizCar` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `cars`
--

DROP TABLE IF EXISTS `cars`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `cars` (
  `id` int NOT NULL AUTO_INCREMENT,
  `name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `price` int DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `cars`
--

LOCK TABLES `cars` WRITE;
/*!40000 ALTER TABLE `cars` DISABLE KEYS */;
/*!40000 ALTER TABLE `cars` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `sysDetailsL01_01`
--

DROP TABLE IF EXISTS `sysDetailsL01_01`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `sysDetailsL01_01` (
  `Id` varchar(40) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `MasterId` varchar(40) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Name` varchar(150) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `ExtraData` json DEFAULT NULL,
  PRIMARY KEY (`Id`),
  KEY `fk_sysDetailsL01_01_MasterId_From_sysMaster_Id` (`MasterId`),
  CONSTRAINT `fk_sysDetailsL01_01_MasterId_From_sysMaster_Id` FOREIGN KEY (`MasterId`) REFERENCES `sysMaster` (`Id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `sysDetailsL01_01`
--

LOCK TABLES `sysDetailsL01_01` WRITE;
/*!40000 ALTER TABLE `sysDetailsL01_01` DISABLE KEYS */;
INSERT INTO `sysDetailsL01_01` VALUES ('1d3e7fa9-61e9-4c45-93de-4ab773c4d3de','55252949-adbe-4960-be26-2247d6dfd411','DetailsL01_01-0','{\"DataL01_01\": 0}'),('2dd30e7a-7036-46a2-9a45-1669f6434123','3e640298-2ab0-45ab-ae71-31b119ec5fb7','DetailsL01_01-1','{\"DataL01_01\": 1}'),('d9cb3b28-48b3-415f-a9cc-0bd1516b622f','3e640298-2ab0-45ab-ae71-31b119ec5fb7','DetailsL01_01-2','{\"DataL01_01\": 2}');
/*!40000 ALTER TABLE `sysDetailsL01_01` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `sysDetailsL01_02`
--

DROP TABLE IF EXISTS `sysDetailsL01_02`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `sysDetailsL01_02` (
  `Id` varchar(40) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `MasterId` varchar(40) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Name` varchar(150) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `ExtraData` json DEFAULT NULL,
  PRIMARY KEY (`Id`),
  KEY `fk_sysDetailsL1_MasterId_From_sysMaster_Id` (`MasterId`),
  CONSTRAINT `fk_sysDetailsL01_02_MasterId_From_sysMaster_Id` FOREIGN KEY (`MasterId`) REFERENCES `sysMaster` (`Id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `sysDetailsL01_02`
--

LOCK TABLES `sysDetailsL01_02` WRITE;
/*!40000 ALTER TABLE `sysDetailsL01_02` DISABLE KEYS */;
INSERT INTO `sysDetailsL01_02` VALUES ('407ced84-446c-4ccf-a77c-6c204641e73a','55252949-adbe-4960-be26-2247d6dfd411','DetailsL01_02-0','{\"DataL01_02\": 0}'),('d1c30254-2e72-4004-804b-09a3cbd15779','3e640298-2ab0-45ab-ae71-31b119ec5fb7','DetailsL01_02-1','{\"DataL01_02\": 1}');
/*!40000 ALTER TABLE `sysDetailsL01_02` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `sysDetailsL02_01`
--

DROP TABLE IF EXISTS `sysDetailsL02_01`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `sysDetailsL02_01` (
  `Id` varchar(40) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `DetailsL01_01Id` varchar(40) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Name` varchar(150) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `ExtraData` json DEFAULT NULL,
  PRIMARY KEY (`Id`),
  KEY `fk_sysDetailsL02_01_DetailsL01_01Id_From_sysDetailsL1_Id` (`DetailsL01_01Id`),
  CONSTRAINT `fk_sysDetailsL02_01_DetailsL01_01Id_From_sysDetailsL1_Id` FOREIGN KEY (`DetailsL01_01Id`) REFERENCES `sysDetailsL01_01` (`Id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `sysDetailsL02_01`
--

LOCK TABLES `sysDetailsL02_01` WRITE;
/*!40000 ALTER TABLE `sysDetailsL02_01` DISABLE KEYS */;
INSERT INTO `sysDetailsL02_01` VALUES ('0e07cb92-6d78-41b0-b8f9-7b98fedd2d8e','2dd30e7a-7036-46a2-9a45-1669f6434123','DetailsL02_01-3','{\"DataL02_01\": 3}'),('2ee433f7-b8ba-4550-8d63-87ddeb234620','d9cb3b28-48b3-415f-a9cc-0bd1516b622f','DetailsL02_01-2','{\"DataL02_01\": 2}'),('7f68a42c-c74d-4e4b-a4bf-7aee9c3eaeee','2dd30e7a-7036-46a2-9a45-1669f6434123','DetailsL02_01-1','{\"DataL02_01\": 1}'),('b2c74a1d-de32-48f3-9093-7e8cd903964e','1d3e7fa9-61e9-4c45-93de-4ab773c4d3de','DetailsL02_01-0','{\"DataL02_01\": 0}');
/*!40000 ALTER TABLE `sysDetailsL02_01` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `sysDetailsL03_01`
--

DROP TABLE IF EXISTS `sysDetailsL03_01`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `sysDetailsL03_01` (
  `Id` varchar(40) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `DetailsL02_01Id` varchar(40) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Name` varchar(150) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `ExtraData` json DEFAULT NULL,
  PRIMARY KEY (`Id`),
  KEY `fk_sysDetailsL03_01_DetailsL02_01Id_From_sysDetailsL02_01Id` (`DetailsL02_01Id`),
  CONSTRAINT `fk_sysDetailsL03_01_DetailsL02_01Id_From_sysDetailsL02_01_Id` FOREIGN KEY (`DetailsL02_01Id`) REFERENCES `sysDetailsL02_01` (`Id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `sysDetailsL03_01`
--

LOCK TABLES `sysDetailsL03_01` WRITE;
/*!40000 ALTER TABLE `sysDetailsL03_01` DISABLE KEYS */;
INSERT INTO `sysDetailsL03_01` VALUES ('3b56e14e-59b7-489f-beb6-d309308a687c','b2c74a1d-de32-48f3-9093-7e8cd903964e','DetailsL03_01-0','{\"DataL03_01\": 0}'),('a2c18611-17db-418c-8e0d-af89c877155d','0e07cb92-6d78-41b0-b8f9-7b98fedd2d8e','DetailsL03_01-1','{\"DataL03_01\": 1}');
/*!40000 ALTER TABLE `sysDetailsL03_01` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `sysMaster`
--

DROP TABLE IF EXISTS `sysMaster`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `sysMaster` (
  `Id` varchar(40) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `Name` varchar(150) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `ExtraData` json DEFAULT NULL,
  PRIMARY KEY (`Id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `sysMaster`
--

LOCK TABLES `sysMaster` WRITE;
/*!40000 ALTER TABLE `sysMaster` DISABLE KEYS */;
INSERT INTO `sysMaster` VALUES ('3e640298-2ab0-45ab-ae71-31b119ec5fb7','Master-1','{\"DataMaster\": 1}'),('55252949-adbe-4960-be26-2247d6dfd411','Master-0','{\"DataMaster\": 0}');
/*!40000 ALTER TABLE `sysMaster` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `sysPerson`
--

DROP TABLE IF EXISTS `sysPerson`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `sysPerson` (
  `Id` varchar(40) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `FirstName` varchar(150) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `LastName` varchar(150) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `ExtraData` json DEFAULT NULL,
  PRIMARY KEY (`Id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `sysPerson`
--

LOCK TABLES `sysPerson` WRITE;
/*!40000 ALTER TABLE `sysPerson` DISABLE KEYS */;
INSERT INTO `sysPerson` VALUES ('1f0a95cc-e9e8-43ad-9325-c367a2b17e29','Process A1','Process A2',NULL),('2b17e295-e9e8-43ad-9325-c367a1f0a95c','Tomás Rafael','Gómez Poggio',NULL),('310a861b-04f2-400f-a10e-05f59eba160c','Process A1','Process A2',NULL),('39c0561f-100c-41df-8882-7cee45989a48','Process A1','Process A2',NULL),('bbcdd1a4-0dfa-4ed7-963a-5d7d9536e3c5','Process A1','Process A2',NULL),('e1dafd1e-5fea-4178-8c8e-af73cb4b08a1','Process A1','Process A2',NULL),('fa840fba-c7fb-4d06-8a1f-94d9a12b49fa','Process A1','Process A2',NULL),('fdf17b75-0dbd-4b8c-9ec9-ef767264214b','Process A1','Process A2',NULL);
/*!40000 ALTER TABLE `sysPerson` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2022-05-27 16:37:30
