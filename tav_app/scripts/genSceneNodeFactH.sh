#!/bin/bash
outputFile=$1
count=0
declare -a subfolders=()
declare -a split=()

#check if file exists, if this is the case delete it
if [ -f $outputFile ];  then
	rm $outputFile
fi


#input are single files
for f in "$@"
do
    if (( $count > 0 )); then
	split=$(echo $f | awk '{split($0,a,"src/")} END { print a[3] }')
	echo "#include <"${split}">" >> $outputFile;
    fi
    (( count++ ))
done

#write the Class beginning
echo "
namespace tav 
{

class SceneNodeFact{
public:
SceneNodeFact(const std::string &sClassName)
{
	msClassName = sClassName;
};
~SceneNodeFact();

SceneNode* Create(sceneData* scd, std::map< std::string, float >* sceneArgs)
{" >> $outputFile
    
#loop again through all files and write the class names        
count=0
count2=0

for f in "$@"
do
    #collect all folders and subfolders
    if (( $count > 0 )); then
	IFS='.' read -a name <<< "$(basename "${f}") "
	printf "          " >> $outputFile
	if [ "$count2" -ne 0 ] ; then
	    printf "} else " >> $outputFile
	fi
	namenosn=${name#"SN"}
	echo "if ( !msClassName.compare(\"${namenosn}\" ) ) { 
			return new $name(scd, sceneArgs);" >> $outputFile;
        (( count2++ ))
    fi
    (( count++ ))
done

#write end
echo "          } else {
		std::cout << \"SceneNodeFact error, Prototype: \" << msClassName << \" not found\" << std::endl;
	}
	return 0;
};

private:
   	std::string msClassName;
};}" >> $outputFile

