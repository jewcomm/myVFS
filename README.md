# myVFS

### Описание файловой системы

Виртуальная файловая система хранит информацию как минимум в двух файлах

В **первом файле (FT)** записывается информация о каждом файле в блоках *FILE TABLE*

Так же в этом файле хранятся данные о расположении содержимого каждого файла в *FILE ALLOCATION TABLE (FAT)* (блоки NEXT_CLAST)

![](https://github.com/jewcomm/myVFS/blob/master/Pic/FT_struct.jpg?raw=true)


Структура каждой записи **FILE TABLE** и **FAT** приведена ниже 

![](https://github.com/jewcomm/myVFS/blob/master/Pic/Structure.jpg?raw=true)


**Во втором и последующих файлах** (CT) записываются блоки (кластеры) с данными каждого файла 

![](https://github.com/jewcomm/myVFS/blob/master/Pic/Clusters.jpg?raw=true)


### Общий пример работы файловой системы 

![](https://github.com/jewcomm/myVFS/blob/master/Pic/Example.jpg?raw=true)
