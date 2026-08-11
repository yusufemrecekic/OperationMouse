# Operation: Mouse Team Workflow

Bu belge küçük bir indie ekibinin Git, GitHub, Git LFS, Unreal Engine ve VS Code ile güvenli biçimde birlikte çalışması için hazırlanmıştır.

## Temel kavramlar

- **Repository:** Projenin dosyalarını ve değişiklik geçmişini tutan Git deposu.
- **Git:** Yerel dosya değişikliklerini ve geçmişi yöneten sürüm kontrol sistemi.
- **GitHub:** Git repository'sinin ekip tarafından paylaşılabildiği uzak servis.
- **Branch:** Bir görevin ana projeden izole çalışma çizgisi.
- **Commit:** Tek bir mantıksal değişikliğin kayıt noktası.
- **Fetch:** Remote'daki yeni bilgileri indirir, çalışma dosyalarını değiştirmez.
- **Pull:** Remote değişikliklerini indirip mevcut branch'e uygular.
- **Push:** Yerel commit'leri remote'a gönderir.
- **Pull Request (PR):** Bir branch'in incelenip `main` ile birleştirilmesi talebi.
- **Merge:** İki geliştirme çizgisini birleştirme işlemi.
- **Merge conflict:** Git'in aynı değişikliği otomatik birleştiremediği durum.
- **Git LFS:** Büyük binary dosyaları normal Git geçmişi yerine LFS depolamasında tutar.

## Branch düzeni

`main` her zaman paylaşılabilir ve mümkün olduğunca stabil tutulur. Gameplay geliştirmesi doğrudan `main` üzerinde yapılmaz.

Kısa ömürlü branch örnekleri:

- `feature/networked-movement`
- `feature/interaction-system`
- `feature/cat-ai`
- `feature/kitchen-graybox`
- `feature/ui-main-menu`
- `fix/client-interaction-replication`
- Codex çalışmaları için varsayılan olarak `codex/<task-name>`

Bir branch tek bir sistem veya düzeltmeye odaklanmalı ve iş tamamlandığında PR ile birleştirilmelidir.

## Normal geliştirici akışı

```powershell
git switch main
git fetch origin
git pull --ff-only origin main
git switch -c feature/task-name
```

1. `main` branch'ine geçilir.
2. `fetch`, remote'daki yeni branch ve commit bilgilerini alır.
3. `pull --ff-only`, yerel `main` üzerinde beklenmedik otomatik merge commit'i oluşmasını engeller.
4. Görev için yeni ve kısa ömürlü bir branch oluşturulur.
5. Unreal ve VS Code içinde yalnızca görev kapsamındaki dosyalar değiştirilir.
6. Oyun/Editor build'i ve ilgili testler çalıştırılır.
7. Değişiklikler kontrol edilip mantıksal commit'lere ayrılır.

```powershell
git status
git diff
git add <dosya-yollari>
git commit -m "feat: describe the focused change"
git push -u origin feature/task-name
```

8. GitHub üzerinde Pull Request açılır.
9. Başka bir ekip üyesi değişikliği inceler ve gerekiyorsa test eder.
10. PR `main` ile birleştirilir; ekip yerel `main` branch'lerini günceller.

## Commit kuralları

Kısa, odaklı ve ne yapıldığını açıklayan mesajlar kullanın:

- `feat: add C++ project module`
- `chore: configure Unreal gitignore`
- `chore: configure Git LFS`
- `docs: add team workflow`
- `feat: add base mouse movement`
- `fix: replicate carried item state`
- `test: verify two-player movement`

`update`, `stuff`, `changes`, `final`, `final2` gibi mesajlar kullanılmamalıdır. Mümkünse her commit tek bir mantıksal değişiklik içermelidir.

## Pull Request kontrol listesi

- [ ] Proje başarıyla build oluyor.
- [ ] İstenen özellik veya düzeltme çalışıyor.
- [ ] İlgiliyse Host + Client testi yapıldı.
- [ ] İlgiliyse gecikme testi yapıldı.
- [ ] İlgisiz asset veya kod değişikliği yok.
- [ ] Generated/cache dosyaları commit'e girmedi.
- [ ] İstenmeyen binary asset conflict'i yok.
- [ ] Yeni hata veya uyarı oluşturulmadı.
- [ ] GDD ve V1 kapsamı korundu.
- [ ] Gerekiyorsa `DEVELOPMENT_STATUS.md` güncellendi.
- [ ] Paylaşılan dosya ve asset değişiklikleri ekiple koordine edildi.

## Text ve Unreal binary dosyaları

`.cpp`, `.h`, `.cs`, `.ini`, `.md`, `.json` ve benzeri text dosyalar satır satır karşılaştırılabilir. Git çoğu bağımsız değişikliği otomatik birleştirebilir; kalan conflict'ler insan tarafından incelenebilir.

`.uasset` ve `.umap` binary Unreal dosyalarıdır. Git bunların içindeki Blueprint node veya map actor değişikliklerini güvenli biçimde satır satır birleştiremez. Binary conflict çoğunlukla sürümlerden birini seçmeyi veya değişikliği Unreal Editor içinde yeniden yapmayı gerektirir. Bu nedenle binary conflict çözmekten önce conflict'i koordinasyonla önlemek gerekir.

## Git LFS kuralları

Repository `.uasset`, `.umap`, `.fbx`, `.blend`, `.psd`, `.tga`, `.exr` ve `.wav` dosyalarını Git LFS ile izler. Her geliştirici Git LFS kurmalı ve clone sonrasında şunu çalıştırmalıdır:

```powershell
git lfs install
git lfs pull
```

LFS normal Git geçmişinin hızla büyümesini engeller; fakat GitHub LFS storage ve bandwidth kotası kullanır. Küçük PNG/JPG referansları ve mevcut küçük GDD normal Git'te kalır. Yeni büyük formatlar otomatik eklenmez; ekip dosya boyutu ve kullanım sıklığını değerlendirir.

Mevcut geçmişi `git lfs migrate` ile yeniden yazmak ancak ekip onayıyla yapılır.

## Unreal asset koordinasyonu

- Aynı Blueprint'i iki kişinin aynı anda düzenlemesinden kaçının.
- Aynı map üzerinde çalışmadan önce sahiplik ve bölge paylaşımını konuşun.
- Ana Character Blueprint'leri, ortak Data Asset'ler, Gameplay Tags ve ana map değişikliklerini duyurun.
- Büyük shared asset düzenlemelerini küçük ve hızlı PR'larla tamamlayın.
- Sistemleri modüler tutarak aynı binary asset'e dokunma ihtiyacını azaltın.

World Partition kullanan büyük haritalarda **One File Per Actor**, birçok actor değişikliğini ayrı dosyalara ayırarak level işbirliğini kolaylaştırabilir. Fakat Level Blueprint, World Settings, Data Layer, ortak Blueprint veya aynı actor üzerindeki conflict'leri otomatik çözmez. Bu özellik koordinasyon ihtiyacını ortadan kaldırmaz.

## Yüksek conflict riski taşıyan alanlar

- GameMode, GameState, PlayerState ve PlayerController
- Mouse Character ve ortak movement ayarları
- Shared Input Mapping Context
- Gameplay Tags ve ortak Data Tables
- Ana mapler ve World Settings
- Önemli Character Blueprint'leri
- Mission state mimarisi

Bu dosyalar yalnızca bir kişinin dokunabileceği dosyalar değildir; değişiklik öncesinde ekip koordinasyonu gerektirir.

## Sorumluluk şablonu

```text
Developer:
Primary Responsibility:
Secondary Responsibility:
Owned / Coordinated Assets:
Current Branch:
Current Task:
```

## Codex ve insan geliştiriciler

Codex bir göreve başlamadan önce branch, `git status` ve mevcut uygulamayı inceler. İlgisiz insan değişikliklerini korur; dosyanın tamamını gereksiz yere biçimlendirmez veya büyük refactor yapmaz.

Görev sonunda şu bilgiler raporlanır:

- Files created, modified, and deleted
- Build result
- Test result
- Known issues

`git reset --hard`, `git clean -fd`, force push, history rewrite veya birleştirilmemiş branch silme işlemleri açık onay olmadan yapılmaz.

## Gizli bilgiler

Parola, token, API key, Steam private credential, kişisel environment dosyası veya makineye özel secret commit edilmez. Hassas ayarlar gerektiğinde ignored yerel dosya veya environment variable kullanılır.

## Günlük örnek: Bir geliştirici bugün Operation: Mouse üzerinde çalışmaya başlayacaksa ne yapacak?

1. Git LFS'in kurulu olduğundan emin olur.
2. Repository klasörünü VS Code ile, `OperationMouse.uproject` dosyasını Unreal Editor ile açar.
3. Terminalde `git status` ile yarım kalmış yerel değişiklik olup olmadığını kontrol eder.
4. `git switch main`, `git fetch origin` ve `git pull --ff-only origin main` çalıştırır.
5. Atanan iş için `git switch -c feature/task-name` ile branch açar.
6. Binary asset değiştirecekse ekipte aynı asset üzerinde çalışan biri olmadığını doğrular.
7. C++ dosyalarını VS Code'da, Blueprint/map/asset dosyalarını Unreal Editor'da düzenler.
8. VS Code'daki varsayılan Unreal build task'ını veya `Scripts/Build.ps1` dosyasını çalıştırır.
9. Gameplay değişikliği ise gerekli solo ve Host + Client testlerini yapar.
10. `git status` ve `git diff` ile yalnızca beklenen dosyaların değiştiğini doğrular.
11. Dosyaları seçerek stage eder ve anlamlı bir commit oluşturur.
12. Branch'i `git push -u origin feature/task-name` ile GitHub'a gönderir.
13. PR açar, kontrol listesini doldurur ve inceleme ister.
14. PR onaylanıp `main` ile birleştirildikten sonra yerel `main` branch'ini yeniden günceller.
