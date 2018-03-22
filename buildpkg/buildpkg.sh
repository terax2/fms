#!/bin/bash -xe

# 변경해야 할 항목 (VER, REL)
# 우선순위
# 1. "buildpkg.sh 1.0 1" 형태로 스크립트 실행 매개변수로 넘기면 그걸 사용
# 2. jenkins parameterized build에서 정의하면 그걸 사용
# 3. 위에 다 안쓰면, 아래 적힌 값으로 사용
if [ -z $1 ]
then
	if [ -z $VER ]
	then
		VER=1.0
	fi
else
	VER=$1
fi
if [ -z $2 ]
then
	if [ -z $REL ]
	then
		REL=1
	fi
else
	REL=$2
fi
if [ -z $3 ]
then 
	if [ -z $ISTEST ]
	then
		ISTEST=0
	fi
else
	ISTEST=$3
fi

if [ $ISTEST != 0 ]
then
	spec_file="rpmbuild_test.spec"
	PROGRAM=FMS_test
else
	spec_file="rpmbuild.spec"
	PROGRAM=FMS
fi

# 자동 수집하는 항목
RPMS_PATH=$(rpm --eval "%{_rpmdir}")
## symbolink link 해서 실행해도 되도록 지원
if test -h $0; then
	symdir=$(dirname "$(ls -l $0 | sed -n 's/.*-> //p')")
	if [[ -z $symdir ]]; then
		symdir=.
	fi
	#fullreldir=$(dirname $0)/$symdir
	fullreldir=$symdir
spec_path="$(cd $fullreldir && pwd)"
else
spec_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)"
fi

fmsd_path="$spec_path"

curdir="$(pwd)"
cd "$curdir"

## os-level architecture명 => 표준 rpm architecture명
# OS-level architecture name, like 'i686'
ARCH=`arch`
# returns the translate line as "arch-from: arch-to"
ARCH=`cat /usr/lib/rpm/rpmrc | grep "buildarchtranslate: $ARCH" | cut -c21-`
# strips the prefix up to colon and following space, returns arch-to.
# Assumes just one space after colon. If not, more regex magic is needed.
ARCH=${ARCH/#*: /}


cd "$curdir"
# localization
#echo "xgettext -dgabia_mond --add-comments=/ --language=shell --from-code=utf-8 --package-name=$PROGRAM --package-version=$VER -p $spec_dir/localization $spec_dir/rpmbuild.spec"
#xgettext -dgabia_mond --add-comments=/ --language=shell --from-code=utf-8 --package-name=$PROGRAM --package-version=$VER -p ./localization ./rpmbuild.spec
#xgettext -dgabia_mond --add-comments=/ --language=shell --from-code=utf-8 --package-name=$PROGRAM --package-version=$VER -p ./localization ./$spec_file
#msgmerge --update localization/ko.po localization/gabia_mond.po
#msgfmt -o localization/ko.mo localization/ko.po
# pyinstaller 로 컴파일 후 rpm 생성
#tcl_ver8_5=$(echo "$(rpm -q --qf "%{VERSION}\n" tcl | awk -F"." '{print $1"."$2}') > 8.4" | bc)
#if [ $tcl_ver8_5 -eq 1 ]
#then
#	rpmmake="rpmmake-tcl8.5"
#else
#	rpmmake="rpmmake-tcl8.4"
#fi
#"$spec_path"/$rpmmake -bb --define "extend_path $spec_path/extend" \
#--define "version $VER" --define "release $REL" --define "name $PROGRAM" --define "agent_path $agent_path" --define "build_path $spec_path" \
#"$spec_path/$spec_file"
#"$spec_path"/rpmbuild.spec

rpmbuild -bb --define "extend_path $fmsd_path/.." --define "version $VER" --define "release $REL" --define "name $PROGRAM" "$fmsd_path"/rpmbuild.spec

# 생성된 rpm 현재 디렉토리로 복사 후 readme 파일과 함께 압축
mkdir -p packages; cd packages
cp -f $RPMS_PATH/$ARCH/$PROGRAM-$VER-$REL.$ARCH.rpm .
# 1. yum repository server로 복사
# 2. createrepo /srv/repo/x86_64 실행
#tar cvfz $PROGRAM-$VER-$REL.$ARCH.tar.gz $PROGRAM-$VER-$REL.$ARCH.rpm
#rm -f $PROGRAM-$VER-$REL.$ARCH.rpm
cd ..
